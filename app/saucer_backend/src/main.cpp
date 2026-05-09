#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>
#include <saucer/embedded/all.hpp>
#include <saucer/script.hpp>
#include <saucer/smartview.hpp>

namespace {

using nlohmann::json;
namespace fs = std::filesystem;

std::string handle_native_request(const std::string &raw);

bool is_env_enabled(const char *name) {
  const char *raw = std::getenv(name);
  if (raw == nullptr) {
    return false;
  }

  const std::string value(raw);
  return !value.empty() && value != "0" && value != "false" &&
         value != "FALSE";
}

coco::stray start(saucer::application *app) {
  auto window = saucer::window::create(app).value();
  auto webview_result = saucer::smartview::create({.window = window});
  if (!webview_result.has_value()) {
    co_return;
  }
  auto webview = std::move(webview_result.value());

  window->set_title("cpp-webview-learn (Saucer)");
  window->set_size({.w = 1200, .h = 800});

  webview.expose("nativeInvoke", [](const std::string &payload) {
    return handle_native_request(payload);
  });

  // Keep current frontend API unchanged: window.nativeInvoke(payload).
  webview.inject({
      .code =
          "window.nativeInvoke = (payload) => saucer.exposed.nativeInvoke(payload);",
      .run_at = saucer::script::time::creation,
  });

  if (is_env_enabled("WEBVIEW_DEVTOOLS")) {
    webview.set_dev_tools(true);
    webview.set_context_menu(true);
  }

  const char *dev_server_url = std::getenv("WEBVIEW_DEV_SERVER_URL");
  if (dev_server_url != nullptr && std::string(dev_server_url).size() > 0) {
    webview.set_url(dev_server_url);
  } else {
    webview.embed(saucer::embedded::all());
    webview.serve("/index.html");
  }

  window->show();
  co_await app->finish();
}

std::string response_error(const std::string &id, const std::string &code,
                           const std::string &message) {
  json payload = {
      {"id", id},
      {"error", {{"code", code}, {"message", message}}},
  };
  return payload.dump();
}

std::string handle_native_request(const std::string &raw) {
  std::string request_id = "unknown";

  try {
    json request = json::parse(raw);

    if (request.contains("id") && request["id"].is_string()) {
      request_id = request["id"].get<std::string>();
    }

    if (!request.contains("method") || !request["method"].is_string()) {
      return response_error(request_id, "INVALID_REQUEST",
                            "missing string field: method");
    }

    const std::string method = request["method"].get<std::string>();
    const json params = (request.contains("params") && request["params"].is_object())
                            ? request["params"]
                            : json::object();

    if (method == "ping") {
      const std::string message =
          params.value("message", std::string("hello from saucer backend"));
      json response = {
          {"id", request_id},
          {"result", {{"echo", message}, {"method", "ping"}}},
      };
      return response.dump();
    }

    if (method == "getAppInfo") {
      json response = {
          {"id", request_id},
          {"result",
           {
               {"name", "cpp-webview-learn"},
               {"backend", "c++23-saucer"},
               {"bridgeVersion", "1.0.0"},
           }},
      };
      return response.dump();
    }

    return response_error(request_id, "METHOD_NOT_FOUND",
                          "unsupported method: " + method);
  } catch (const std::exception &ex) {
    return response_error(request_id, "INVALID_JSON", ex.what());
  }
}

} // namespace

int main() {
  return saucer::application::create({.id = "cpp-webview-learn-saucer"})
      ->run(start);
}
