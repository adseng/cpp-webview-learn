import { useState } from "react";
import { invokeNative } from "./bridge/native";

type UiState = {
  title: string;
  content: string;
};

const initialState: UiState = {
  title: "Ready",
  content: "Click a button to call native backend."
};

function makeRequestId(prefix: string): string {
  return `${prefix}-${Date.now()}`;
}

export default function App() {
  const [state, setState] = useState<UiState>(initialState);

  async function callPing() {
    const response = await invokeNative({
      id: makeRequestId("ping"),
      method: "ping",
      params: { message: "hello from react" }
    });

    if (response.error) {
      setState({
        title: "Ping failed",
        content: `${response.error.code}: ${response.error.message}`
      });
      return;
    }

    setState({
      title: "Ping success",
      content: JSON.stringify(response.result)
    });
  }

  async function callAppInfo() {
    const response = await invokeNative({
      id: makeRequestId("appInfo"),
      method: "getAppInfo"
    });

    if (response.error) {
      setState({
        title: "getAppInfo failed",
        content: `${response.error.code}: ${response.error.message}`
      });
      return;
    }

    setState({
      title: "App info",
      content: JSON.stringify(response.result)
    });
  }

  return (
    <main className="min-h-screen bg-slate-950 px-6 py-10 text-slate-100">
      <div className="mx-auto flex w-full max-w-2xl flex-col gap-6">
        <h1 className="text-2xl font-semibold">C++ Webview Bridge Demo</h1>
        <p className="text-sm text-slate-300">
          Frontend invokes native methods through JSON messages.
        </p>
        <div className="flex gap-3">
          <button
            type="button"
            onClick={callPing}
            className="rounded-md bg-indigo-500 px-4 py-2 text-sm font-medium hover:bg-indigo-400"
          >
            Invoke ping
          </button>
          <button
            type="button"
            onClick={callAppInfo}
            className="rounded-md bg-emerald-500 px-4 py-2 text-sm font-medium hover:bg-emerald-400"
          >
            Invoke getAppInfo
          </button>
        </div>
        <section className="rounded-md border border-slate-700 bg-slate-900 p-4">
          <h2 className="text-sm font-semibold text-slate-200">{state.title}</h2>
          <pre className="mt-2 overflow-auto whitespace-pre-wrap break-words text-xs text-slate-300">
            {state.content}
          </pre>
        </section>
      </div>
    </main>
  );
}
