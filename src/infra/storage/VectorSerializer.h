#ifndef VECTOR_SERIALIZER_H
#define VECTOR_SERIALIZER_H

#include <vector>
#include <string>

class VectorSerializer {
public:
    /**
     * 将浮点数向量序列化为二进制字符串
     * @param vec 浮点数向量
     * @return 序列化后的二进制字符串
     */
    static std::string serialize(const std::vector<float>& vec);
    
    /**
     * 将二进制字符串反序列化为浮点数向量
     * @param data 二进制字符串
     * @return 反序列化后的浮点数向量
     */
    static std::vector<float> deserialize(const std::string& data);
};

#endif // VECTOR_SERIALIZER_H