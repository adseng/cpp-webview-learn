type NativeInvoker = (payload: string) => Promise<unknown>;

declare global {
  interface Window {
    nativeInvoke?: NativeInvoker;
  }
}

export type NativeRequest = {
  id: string;
  method: string;
  params?: Record<string, unknown>;
};

export type NativeResponse = {
  id: string;
  result?: unknown;
  error?: { code: string; message: string };
};

export async function invokeNative(
  request: NativeRequest
): Promise<NativeResponse> {
  if (!window.nativeInvoke) {
    return {
      id: request.id,
      error: { code: "BRIDGE_UNAVAILABLE", message: "native bridge not ready" }
    };
  }

  const raw = await window.nativeInvoke(JSON.stringify(request));
  if (typeof raw === "string") {
    try {
      return JSON.parse(raw) as NativeResponse;
    } catch {
      return {
        id: request.id,
        error: {
          code: "INVALID_RESPONSE",
          message: "native returned non-JSON string response"
        }
      };
    }
  }

  if (raw && typeof raw === "object") {
    return raw as NativeResponse;
  }

  return {
    id: request.id,
    error: {
      code: "INVALID_RESPONSE",
      message: `native returned unsupported response type: ${typeof raw}`
    }
  };
}
