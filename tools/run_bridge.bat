@echo off
setlocal
cd /d "%~dp0.."
python tools\signal_bridge_run.py %*
