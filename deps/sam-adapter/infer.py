import sys
sys.path.append('../deps/sam-adapter/SAM-Adapter-PyTorch')

import argparse

import yaml
import torch
from torch.utils.data import DataLoader

import datasets
import models

import numpy as np
from PIL import Image


device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--config')
    parser.add_argument('--model')
    args = parser.parse_args()

    with open(args.config, 'r') as f:
        config = yaml.load(f, Loader=yaml.FullLoader)
    spec = config['test_dataset']
    dataset = datasets.make(spec['dataset'])
    dataset = datasets.make(spec['wrapper'], args={'dataset': dataset})
    loader = DataLoader(dataset, batch_size=spec['batch_size'],
                        num_workers=8)

    model = models.make(config['model']).cuda()
    sam_checkpoint = torch.load(args.model, map_location='cuda:0')
    model.load_state_dict(sam_checkpoint, strict=True)
    
    with torch.no_grad():
        model.eval()

        number = 1
        for batch in loader:
            for k, v in batch.items():
                batch[k] = v.cuda()
            inp = batch['inp']

            pred = torch.sigmoid(model.infer(inp))
            pred_np = pred.cpu().detach().numpy()
            pred_np = np.uint8(255 * (pred_np > 0.5))
            pred_img = Image.fromarray(pred_np.reshape(1024, 1024))
            pred_img.save('../DataIO/OutputMasks/' + str(number) + '.png')
            number += 1
