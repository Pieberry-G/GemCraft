%1 mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit
@echo off
cd /d %~dp0

cmake -B build

conda info --envs | findstr /I /C:"sam-adapter" >nul
if errorlevel 1 (
    echo sam-adapter environment not found. Creating sam-adapter environment...
    set https_proxy=http://127.0.0.1:7890
    conda create -n sam-adapter -y python=3.8
    conda activate sam-adapter
    pip install torch==2.3.0 torchvision==0.18.0 --index-url https://download.pytorch.org/whl/cu121
    pip install -r deps/sam-adapter/Setting/requirements.txt
) else (
    echo sam-adapter environment already exists.
)

PAUSE
