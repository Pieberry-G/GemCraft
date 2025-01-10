%1 mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit
@echo off
cd /d %~dp0

cmake -B build

conda info --envs | findstr /I /C:"GCSamAdapter" >nul
if errorlevel 1 (
    echo sam-adapter environment not found. Creating GCSamAdapter environment...
    set https_proxy=http://127.0.0.1:7890
    conda create -n GCSamAdapter -y python=3.8
    conda activate GCSamAdapter
    pip install torch==2.3.0 torchvision==0.18.0 --index-url https://download.pytorch.org/whl/cu121
    pip install -r deps/sam-adapter/requirements.txt
) else (
    echo GCSamAdapter environment already exists.
)

PAUSE
