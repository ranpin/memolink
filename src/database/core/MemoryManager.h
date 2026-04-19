#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include "ORMWrapper.h"
#include <optional>
#include <vector>

/**
 * 通用记忆管理器模板类
 * @tparam T 记忆模型类型
 */
template<typename T>
class MemoryManager {
protected:
    ORMWrapper& orm_;

public:
    explicit MemoryManager(ORMWrapper& orm) : orm_(orm) {}
    
    /**
     * 添加一个新的记忆实体
     * @param memory 记忆对象
     * @return 插入的ID，失败返回-1
     */
    int64_t add(const T& memory) {
        return orm_.insert<T>(memory);
    }
    
    /**
     * 通过ID获取单个记忆
     * @param id 记忆ID
     * @return 记忆对象或空值
     */
    std::optional<T> getById(int64_t id) {
        return orm_.getById<T>(id);
    }
    
    /**
     * 获取所有记忆
     * @return 记忆对象列表
     */
    std::vector<T> getAll() {
        return orm_.getAll<T>();
    }
    
    /**
     * 根据条件查询记忆实体
     * @param condition 查询条件
     * @return 符合条件的记忆对象列表
     */
    template<typename Condition>
    std::vector<T> find(Condition condition) {
        return orm_.getByCondition<T>(condition);
    }
    
    /**
     * 更新记忆实体
     * @param memory 记忆对象
     * @return 是否更新成功
     */
    bool update(const T& memory) {
        try {
            orm_.update<T>(memory);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    /**
     * 根据ID删除记忆实体
     * @param id 记忆ID
     */
    void remove(int64_t id) {
        orm_.remove<T>(id);
    }
    
    /**
     * 根据用户ID查询记忆
     * @param userId 用户ID
     * @return 该用户的所有记忆
     */
    std::vector<T> getByUserId(int64_t userId) {
        return find(sqlite_orm::where(sqlite_orm::c(&T::user_id) == userId));
    }
};

#endif // MEMORYMANAGER_H