# 文档

[开发说明]('./docs/开发说明.md')

[通信协议]('./docs/通信协议.md')

# 启动

```powershell
pnpm frontend:build
Remove-Item Env:WEBVIEW_DEV_SERVER_URL -ErrorAction SilentlyContinue
.\app\backend\build\Release\cpp_webview_host.exe
```