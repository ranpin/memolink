#!/usr/bin/env python3

import sys
import os
import json
import base64
import torch
import clip
from PIL import Image
import numpy as np

def process_image(image_path):
    """处理图像并生成CLIP embedding"""
    try:
        # 加载CLIP模型
        device = "cuda" if torch.cuda.is_available() else "cpu"
        model, preprocess = clip.load("ViT-B/32", device=device)
        
        # 加载并预处理图像
        image = Image.open(image_path)
        image_input = preprocess(image).unsqueeze(0).to(device)
        
        # 生成embedding
        with torch.no_grad():
            image_features = model.encode_image(image_input)
            # 归一化特征向量
            image_features /= image_features.norm(dim=-1, keepdim=True)
            
        # 转换为numpy数组并返回
        features = image_features.cpu().numpy().flatten().tolist()
        return {"success": True, "embedding": features}
    except Exception as e:
        return {"success": False, "error": str(e)}

def process_text(text):
    """处理文本并生成CLIP embedding"""
    try:
        # 加载CLIP模型
        device = "cuda" if torch.cuda.is_available() else "cpu"
        model, preprocess = clip.load("ViT-B/32", device=device)
        
        # 处理文本
        text_input = clip.tokenize([text]).to(device)
        
        # 生成embedding
        with torch.no_grad():
            text_features = model.encode_text(text_input)
            # 归一化特征向量
            text_features /= text_features.norm(dim=-1, keepdim=True)
            
        # 转换为numpy数组并返回
        features = text_features.cpu().numpy().flatten().tolist()
        return {"success": True, "embedding": features}
    except Exception as e:
        return {"success": False, "error": str(e)}

def main():
    if len(sys.argv) < 3:
        print(json.dumps({"success": False, "error": "参数不足"}))
        sys.exit(1)
    
    mode = sys.argv[1]  # "image" 或 "text"
    input_data = sys.argv[2]  # 图像路径或文本内容
    
    if mode == "image":
        result = process_image(input_data)
    elif mode == "text":
        result = process_text(input_data)
    else:
        result = {"success": False, "error": "不支持的模式"}
    
    print(json.dumps(result))
    sys.exit(0 if result["success"] else 1)

if __name__ == "__main__":
    main()