@echo off

call conda activate sam-adapter
python ../deps/sam-adapter/Setting/infer.py --config ../deps/sam-adapter/Setting/configs/gemcraft-sam-vit-l.yaml --model ../deps/sam-adapter/Setting/save/_gemcraft-sam-vit-l/model_epoch_last.pth
conda deactivate
