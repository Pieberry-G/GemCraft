@echo off

call conda activate GCSamAdapter
python ../deps/sam-adapter/infer.py --config ../deps/sam-adapter/configs/gemcraft-sam-vit-l.yaml --model ../deps/sam-adapter/pretrained/model_epoch_last.pth
conda deactivate
