#!/usr/bin/env python3

import torch
import clip
import numpy as np
import onnx
import onnxruntime
import os
from torch.onnx import export

def download_and_convert_clip_model():
    """
    下载CLIP模型并转换为ONNX格式
    """
    print("正在下载CLIP模型...")
    
    # 加载CLIP模型
    model, preprocess = clip.load("ViT-B/32", device="cpu")
    model.eval()
    
    print("模型加载完成")
    
    # 创建示例输入
    dummy_image = torch.randn(1, 3, 224, 224)
    dummy_text = torch.randint(0, 100, (1, 77)).long()
    
    # 确保模型目录存在
    os.makedirs("models", exist_ok=True)
    
    # 导出图像编码器
    print("正在导出图像编码器...")
    torch.onnx.export(
        model.visual,
        dummy_image,
        "models/clip_image_encoder.onnx",
        export_params=True,
        opset_version=11,
        do_constant_folding=True,
        input_names=['image'],
        output_names=['image_features'],
        dynamic_axes={
            'image': {0: 'batch_size'},
            'image_features': {0: 'batch_size'}
        }
    )
    
    # 导出文本编码器（简化版）
    print("正在导出文本编码器...")
    # 注意：这里需要特殊处理，因为CLIP的文本编码器包含额外的组件
    # 我们导出整个模型，然后在推理时只使用文本部分
    torch.onnx.export(
        model,
        (dummy_image, dummy_text),
        "models/clip_model.onnx",
        export_params=True,
        opset_version=11,
        do_constant_folding=True,
        input_names=['image', 'text'],
        output_names=['image_features', 'text_features'],
        dynamic_axes={
            'image': {0: 'batch_size'},
            'text': {0: 'batch_size', 1: 'sequence'},
            'image_features': {0: 'batch_size'},
            'text_features': {0: 'batch_size'}
        }
    )
    
    print("模型导出完成!")
    print("模型文件已保存到 models/ 目录")

if __name__ == "__main__":
    download_and_convert_clip_model()