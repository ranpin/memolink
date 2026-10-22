#import "ScreenCaptureHelper.h"
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#include <iostream>
#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif

bool capture_screen_to_file(const char* file_path) {
    @autoreleasepool {
        // 使用系统命令捕获屏幕截图
        NSString *filePath = [NSString stringWithUTF8String:file_path];
        NSString *command = [NSString stringWithFormat:@"screencapture -x %@", filePath];
        
        int result = system([command UTF8String]);
        
        if (result == 0) {
            std::cout << "屏幕截图保存成功: " << file_path << std::endl;
            return true;
        } else {
            std::cerr << "屏幕截图保存失败，系统命令执行错误" << std::endl;
            return false;
        }
    }
}

#ifdef __cplusplus
}
#endif

