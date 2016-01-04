@IF EXIST "%~dp0\node.exe" (
  "%~dp0\node.exe" "%~dp0\..\lib\node_modules\npm\node_modules\node-gyp\bin\node-gyp.js" %*
) ELSE (
  node "%~dp0\..\lib\node_modules\npm\node_modules\node-gyp\bin\node-gyp.js" %*
)
