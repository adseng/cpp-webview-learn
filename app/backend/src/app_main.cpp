#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "webview/webview.h"

namespace {

using nlohmann::json;
namespace fs = std::filesystem;

std::string response_error(const std::string &id, const std::string &code,
                           const std::string &message) {
  json payload = {
      {"id", id},
      {"error", {{"code", code}, {"message", message}}},
  };
  return payload.dump();
}

std::optional<std::string> read_file_binary(const fs::path &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return std::nullopt;
  }

  std::ostringstream stream;
  stream << file.rdbuf();
  return stream.str();
}

class StaticFileServer {
public:
  ~StaticFileServer() { stop(); }

  bool start(const fs::path &root_dir, std::optional<uint16_t> preferred_port) {
    if (running_) {
      return true;
    }

    root_dir_ = fs::weakly_canonical(root_dir);
    if (!fs::exists(root_dir_ / "index.html")) {
      return false;
    }

    if (!server_.set_mount_point("/", root_dir_.string())) {
      return false;
    }

    // Keep client-side routing working in production.
    server_.set_error_handler(
        [this](const httplib::Request &request, httplib::Response &response) {
          if (response.status != 404) {
            return;
          }
          if (fs::path(request.path).has_extension()) {
            return;
          }

          const auto index_html = read_file_binary(root_dir_ / "index.html");
          if (!index_html.has_value()) {
            return;
          }
          response.status = 200;
          response.set_content(*index_html, "text/html; charset=utf-8");
        });

    int bound_port = -1;
    if (preferred_port.has_value()) {
      bound_port = server_.bind_to_port("127.0.0.1", *preferred_port);
    }
    if (bound_port <= 0) {
      bound_port = server_.bind_to_any_port("127.0.0.1");
    }
    if (bound_port <= 0) {
      return false;
    }

    port_ = static_cast<uint16_t>(bound_port);
    running_ = true;
    server_thread_ = std::thread([this]() { server_.listen_after_bind(); });
    return true;
  }

  void stop() {
    if (!running_) {
      return;
    }
    server_.stop();
    if (server_thread_.joinable()) {
      server_thread_.join();
    }
    running_ = false;
    port_ = 0;
  }

  uint16_t port() const { return port_; }

private:
  httplib::Server server_;
  fs::path root_dir_;
  std::thread server_thread_;
  bool running_ = false;
  uint16_t port_ = 0;
};

std::optional<uint16_t> parse_static_port_env() {
  const char *raw = std::getenv("WEBVIEW_STATIC_PORT");
  if (raw == nullptr || std::string(raw).empty()) {
    return std::nullopt;
  }

  char *end = nullptr;
  const long parsed = std::strtol(raw, &end, 10);
  if (*end != '\0' || parsed <= 0 || parsed > 65535) {
    return std::nullopt;
  }
  return static_cast<uint16_t>(parsed);
}

std::string handle_native_request(const std::string &raw) {
  std::string request_id = "unknown";
  try {
    json request = json::parse(raw);

    // webview.bind callback receives an arguments array by default.
    // We pass one payload argument from frontend, so unwrap index 0 here.
    if (request.is_array()) {
      if (request.empty()) {
        return response_error(request_id, "INVALID_REQUEST",
                              "missing payload argument");
      }
      const json &first_arg = request[0];
      if (first_arg.is_string()) {
        request = json::parse(first_arg.get<std::string>());
      } else if (first_arg.is_object()) {
        request = first_arg;
      } else {
        return response_error(request_id, "INVALID_REQUEST",
                              "payload must be JSON string or object");
      }
    }

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
          params.value("message", std::string("hello from backend"));
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
               {"backend", "c++17-webview"},
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
  webview::webview window(true, nullptr);
  window.set_title("cpp-webview-learn");
  window.set_size(1200, 800, WEBVIEW_HINT_NONE);
  window.bind("nativeInvoke", [](std::string payload) {
    return handle_native_request(payload);
  });

  StaticFileServer file_server;
  const char *dev_server_url = std::getenv("WEBVIEW_DEV_SERVER_URL");
  if (dev_server_url != nullptr && std::string(dev_server_url).size() > 0) {
    window.navigate(dev_server_url);
  } else {
    const fs::path dist_index(FRONTEND_DIST_INDEX);
    const fs::path dist_root = dist_index.parent_path();
    if (fs::exists(dist_index) &&
        file_server.start(dist_root, parse_static_port_env())) {
      const std::string static_url =
          "http://127.0.0.1:" + std::to_string(file_server.port()) + "/index.html";
      window.navigate(static_url);
    } else {
      window.set_html(
          "<html><body style='font-family:Segoe UI;padding:16px;background:#020617;"
          "color:#e2e8f0'>frontend build missing or static server unavailable</body></html>");
    }
  }

  window.run();
  return 0;
}
