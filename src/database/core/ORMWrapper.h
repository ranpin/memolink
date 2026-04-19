#ifndef ORMWRAPPER_H
#define ORMWRAPPER_H

#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <stdexcept>
#include <functional>
#include "../models/MemoryModels.h"

class ORMWrapper {
private:
    // 使用sqlite_orm的storage类型
    using Storage = decltype(sqlite_orm::make_storage("",
        sqlite_orm::make_table("users",
            sqlite_orm::make_column("id", &User::id, sqlite_orm::primary_key().autoincrement()),
            sqlite_orm::make_column("name", &User::name),
            sqlite_orm::make_column("email", &User::email),
            sqlite_orm::make_column("password_hash", &User::password_hash),
            sqlite_orm::make_column("created_at", &User::created_at)
        ),
        sqlite_orm::make_table("message_memories",
            sqlite_orm::make_column("id", &MessageMemory::id, sqlite_orm::primary_key().autoincrement()),
            sqlite_orm::make_column("user_id", &MessageMemory::user_id),
            sqlite_orm::make_column("agent_id", &MessageMemory::agent_id),
            sqlite_orm::make_column("session_id", &MessageMemory::session_id),
            sqlite_orm::make_column("content", &MessageMemory::content),
            sqlite_orm::make_column("created_at", &MessageMemory::created_at),
            sqlite_orm::make_column("processed", &MessageMemory::processed)
        ),
        sqlite_orm::make_table("episodic_memories",
            sqlite_orm::make_column("id", &EpisodicMemory::id, sqlite_orm::primary_key().autoincrement()),
            sqlite_orm::make_column("user_id", &EpisodicMemory::user_id),
            sqlite_orm::make_column("agent_id", &EpisodicMemory::agent_id),
            sqlite_orm::make_column("session_id", &EpisodicMemory::session_id),
            sqlite_orm::make_column("event_description", &EpisodicMemory::event_description),
            sqlite_orm::make_column("participants", &EpisodicMemory::participants),
            sqlite_orm::make_column("location", &EpisodicMemory::location),
            sqlite_orm::make_column("timestamp", &EpisodicMemory::timestamp),
            sqlite_orm::make_column("importance", &EpisodicMemory::importance),
            sqlite_orm::make_column("metadata", &EpisodicMemory::metadata),
            sqlite_orm::make_column("created_at", &EpisodicMemory::created_at)
        ),
        sqlite_orm::make_table("semantic_memories",
            sqlite_orm::make_column("id", &SemanticMemory::id, sqlite_orm::primary_key().autoincrement()),
            sqlite_orm::make_column("user_id", &SemanticMemory::user_id),
            sqlite_orm::make_column("agent_id", &SemanticMemory::agent_id),
            sqlite_orm::make_column("fact", &SemanticMemory::fact),
            sqlite_orm::make_column("category", &SemanticMemory::category),
            sqlite_orm::make_column("confidence", &SemanticMemory::confidence),
            sqlite_orm::make_column("metadata", &SemanticMemory::metadata),
            sqlite_orm::make_column("created_at", &SemanticMemory::created_at)
        ),
        sqlite_orm::make_table("procedural_memories",
            sqlite_orm::make_column("id", &ProceduralMemory::id, sqlite_orm::primary_key().autoincrement()),
            sqlite_orm::make_column("user_id", &ProceduralMemory::user_id),
            sqlite_orm::make_column("agent_id", &ProceduralMemory::agent_id),
            sqlite_orm::make_column("name", &ProceduralMemory::name),
            sqlite_orm::make_column("description", &ProceduralMemory::description),
            sqlite_orm::make_column("steps", &ProceduralMemory::steps),
            sqlite_orm::make_column("metadata", &ProceduralMemory::metadata),
            sqlite_orm::make_column("created_at", &ProceduralMemory::created_at)
        ),
        sqlite_orm::make_table("secret_vault",
            sqlite_orm::make_column("id", &SecretVault::id, sqlite_orm::primary_key().autoincrement()),
            sqlite_orm::make_column("user_id", &SecretVault::user_id),
            sqlite_orm::make_column("agent_id", &SecretVault::agent_id),
            sqlite_orm::make_column("category", &SecretVault::category),
            sqlite_orm::make_column("title", &SecretVault::title),
            sqlite_orm::make_column("content", &SecretVault::content),
            sqlite_orm::make_column("metadata", &SecretVault::metadata),
            sqlite_orm::make_column("created_at", &SecretVault::created_at)
        ),
        sqlite_orm::make_table("core_memories",
            sqlite_orm::make_column("id", &CoreMemory::id, sqlite_orm::primary_key().autoincrement()),
            sqlite_orm::make_column("user_id", &CoreMemory::user_id),
            sqlite_orm::make_column("agent_id", &CoreMemory::agent_id),
            sqlite_orm::make_column("memory_type", &CoreMemory::memory_type),
            sqlite_orm::make_column("content", &CoreMemory::content),
            sqlite_orm::make_column("priority", &CoreMemory::priority),
            sqlite_orm::make_column("metadata", &CoreMemory::metadata),
            sqlite_orm::make_column("created_at", &CoreMemory::created_at)
        ),
        sqlite_orm::make_table("multimodal_memories",
            sqlite_orm::make_column("id", &MultimodalMemory::id, sqlite_orm::primary_key().autoincrement()),
            sqlite_orm::make_column("user_id", &MultimodalMemory::user_id),
            sqlite_orm::make_column("session_id", &MultimodalMemory::session_id),
            sqlite_orm::make_column("data_type", &MultimodalMemory::data_type),
            sqlite_orm::make_column("file_path", &MultimodalMemory::file_path),
            sqlite_orm::make_column("description", &MultimodalMemory::description),
            sqlite_orm::make_column("metadata", &MultimodalMemory::metadata),
            sqlite_orm::make_column("created_at", &MultimodalMemory::created_at)
        ),
        sqlite_orm::make_table("memory_provenance",
            sqlite_orm::make_column("id", &MemoryProvenance::id, sqlite_orm::primary_key().autoincrement()),
            sqlite_orm::make_column("memory_type", &MemoryProvenance::memory_type),
            sqlite_orm::make_column("memory_id", &MemoryProvenance::memory_id),
            sqlite_orm::make_column("source_type", &MemoryProvenance::source_type),
            sqlite_orm::make_column("source_id", &MemoryProvenance::source_id),
            sqlite_orm::make_column("relation_type", &MemoryProvenance::relation_type),
            sqlite_orm::make_column("created_at", &MemoryProvenance::created_at)
        ),
        sqlite_orm::make_table("sessions",
            sqlite_orm::make_column("id", &Session::id, sqlite_orm::primary_key().autoincrement()),
            sqlite_orm::make_column("user_id", &Session::user_id),
            sqlite_orm::make_column("start_time", &Session::start_time),
            sqlite_orm::make_column("end_time", &Session::end_time),
            sqlite_orm::make_column("metadata", &Session::metadata)
        )
    ));
    
    std::unique_ptr<Storage> storage_;
    sqlite3* db_;

public:
    explicit ORMWrapper(const std::string& db_path, sqlite3* db = nullptr) : db_(db) {
        try {
            // 创建数据库连接和表结构
            storage_ = std::make_unique<Storage>(sqlite_orm::make_storage(
                db_path,
                sqlite_orm::make_table("users",
                    sqlite_orm::make_column("id", &User::id, sqlite_orm::primary_key().autoincrement()),
                    sqlite_orm::make_column("name", &User::name),
                    sqlite_orm::make_column("email", &User::email),
                    sqlite_orm::make_column("password_hash", &User::password_hash),
                    sqlite_orm::make_column("created_at", &User::created_at)
                ),
                sqlite_orm::make_table("message_memories",
                    sqlite_orm::make_column("id", &MessageMemory::id, sqlite_orm::primary_key().autoincrement()),
                    sqlite_orm::make_column("user_id", &MessageMemory::user_id),
                    sqlite_orm::make_column("agent_id", &MessageMemory::agent_id),
                    sqlite_orm::make_column("session_id", &MessageMemory::session_id),
                    sqlite_orm::make_column("content", &MessageMemory::content),
                    sqlite_orm::make_column("created_at", &MessageMemory::created_at),
                    sqlite_orm::make_column("processed", &MessageMemory::processed)
                ),
                sqlite_orm::make_table("episodic_memories",
                    sqlite_orm::make_column("id", &EpisodicMemory::id, sqlite_orm::primary_key().autoincrement()),
                    sqlite_orm::make_column("user_id", &EpisodicMemory::user_id),
                    sqlite_orm::make_column("agent_id", &EpisodicMemory::agent_id),
                    sqlite_orm::make_column("session_id", &EpisodicMemory::session_id),
                    sqlite_orm::make_column("event_description", &EpisodicMemory::event_description),
                    sqlite_orm::make_column("participants", &EpisodicMemory::participants),
                    sqlite_orm::make_column("location", &EpisodicMemory::location),
                    sqlite_orm::make_column("timestamp", &EpisodicMemory::timestamp),
                    sqlite_orm::make_column("importance", &EpisodicMemory::importance),
                    sqlite_orm::make_column("metadata", &EpisodicMemory::metadata),
                    sqlite_orm::make_column("created_at", &EpisodicMemory::created_at)
                ),
                sqlite_orm::make_table("semantic_memories",
                    sqlite_orm::make_column("id", &SemanticMemory::id, sqlite_orm::primary_key().autoincrement()),
                    sqlite_orm::make_column("user_id", &SemanticMemory::user_id),
                    sqlite_orm::make_column("agent_id", &SemanticMemory::agent_id),
                    sqlite_orm::make_column("fact", &SemanticMemory::fact),
                    sqlite_orm::make_column("category", &SemanticMemory::category),
                    sqlite_orm::make_column("confidence", &SemanticMemory::confidence),
                    sqlite_orm::make_column("metadata", &SemanticMemory::metadata),
                    sqlite_orm::make_column("created_at", &SemanticMemory::created_at)
                ),
                sqlite_orm::make_table("procedural_memories",
                    sqlite_orm::make_column("id", &ProceduralMemory::id, sqlite_orm::primary_key().autoincrement()),
                    sqlite_orm::make_column("user_id", &ProceduralMemory::user_id),
                    sqlite_orm::make_column("agent_id", &ProceduralMemory::agent_id),
                    sqlite_orm::make_column("name", &ProceduralMemory::name),
                    sqlite_orm::make_column("description", &ProceduralMemory::description),
                    sqlite_orm::make_column("steps", &ProceduralMemory::steps),
                    sqlite_orm::make_column("metadata", &ProceduralMemory::metadata),
                    sqlite_orm::make_column("created_at", &ProceduralMemory::created_at)
                ),
                sqlite_orm::make_table("secret_vault",
                    sqlite_orm::make_column("id", &SecretVault::id, sqlite_orm::primary_key().autoincrement()),
                    sqlite_orm::make_column("user_id", &SecretVault::user_id),
                    sqlite_orm::make_column("agent_id", &SecretVault::agent_id),
                    sqlite_orm::make_column("category", &SecretVault::category),
                    sqlite_orm::make_column("title", &SecretVault::title),
                    sqlite_orm::make_column("content", &SecretVault::content),
                    sqlite_orm::make_column("metadata", &SecretVault::metadata),
                    sqlite_orm::make_column("created_at", &SecretVault::created_at)
                ),
                sqlite_orm::make_table("core_memories",
                    sqlite_orm::make_column("id", &CoreMemory::id, sqlite_orm::primary_key().autoincrement()),
                    sqlite_orm::make_column("user_id", &CoreMemory::user_id),
                    sqlite_orm::make_column("agent_id", &CoreMemory::agent_id),
                    sqlite_orm::make_column("memory_type", &CoreMemory::memory_type),
                    sqlite_orm::make_column("content", &CoreMemory::content),
                    sqlite_orm::make_column("priority", &CoreMemory::priority),
                    sqlite_orm::make_column("metadata", &CoreMemory::metadata),
                    sqlite_orm::make_column("created_at", &CoreMemory::created_at)
                ),
                sqlite_orm::make_table("multimodal_memories",
                    sqlite_orm::make_column("id", &MultimodalMemory::id, sqlite_orm::primary_key().autoincrement()),
                    sqlite_orm::make_column("user_id", &MultimodalMemory::user_id),
                    sqlite_orm::make_column("session_id", &MultimodalMemory::session_id),
                    sqlite_orm::make_column("data_type", &MultimodalMemory::data_type),
                    sqlite_orm::make_column("file_path", &MultimodalMemory::file_path),
                    sqlite_orm::make_column("description", &MultimodalMemory::description),
                    sqlite_orm::make_column("metadata", &MultimodalMemory::metadata),
                    sqlite_orm::make_column("created_at", &MultimodalMemory::created_at)
                ),
                sqlite_orm::make_table("memory_provenance",
                    sqlite_orm::make_column("id", &MemoryProvenance::id, sqlite_orm::primary_key().autoincrement()),
                    sqlite_orm::make_column("memory_type", &MemoryProvenance::memory_type),
                    sqlite_orm::make_column("memory_id", &MemoryProvenance::memory_id),
                    sqlite_orm::make_column("source_type", &MemoryProvenance::source_type),
                    sqlite_orm::make_column("source_id", &MemoryProvenance::source_id),
                    sqlite_orm::make_column("relation_type", &MemoryProvenance::relation_type),
                    sqlite_orm::make_column("created_at", &MemoryProvenance::created_at)
                ),
                sqlite_orm::make_table("sessions",
                    sqlite_orm::make_column("id", &Session::id, sqlite_orm::primary_key().autoincrement()),
                    sqlite_orm::make_column("user_id", &Session::user_id),
                    sqlite_orm::make_column("start_time", &Session::start_time),
                    sqlite_orm::make_column("end_time", &Session::end_time),
                    sqlite_orm::make_column("metadata", &Session::metadata)
                )
            ));
            
            // 同步表结构
            storage_->sync_schema();
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to initialize ORM storage: " + std::string(e.what()));
        }
    }
    
    // 通用模板方法
    template<typename T>
    int insert(const T& item) {
        try {
            return storage_->insert(item);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to insert item: " + std::string(e.what()));
        }
    }
    
    template<typename T>
    std::optional<T> getById(int id) {
        try {
            auto item = storage_->get_pointer<T>(id);
            if (item) {
                return *item;
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to get item by id: " + std::string(e.what()));
        }
    }
    
    template<typename T>
    std::vector<T> getAll() {
        try {
            return storage_->get_all<T>();
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to get all items: " + std::string(e.what()));
        }
    }
    
    template<typename T, typename Condition>
    std::vector<T> getByCondition(const Condition& condition) {
        try {
            return storage_->get_all<T>(condition);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to get items by condition: " + std::string(e.what()));
        }
    }
    
    template<typename T>
    void update(const T& item) {
        try {
            storage_->update(item);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to update item: " + std::string(e.what()));
        }
    }
    
    template<typename T>
    void remove(int id) {
        try {
            storage_->remove<T>(id);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to remove item: " + std::string(e.what()));
        }
    }
    
    // User相关操作
    int insertUser(const User& user) {
        try {
            return storage_->insert(user);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to insert user: " + std::string(e.what()));
        }
    }
    
    std::optional<User> getUserById(int id) {
        try {
            auto user = storage_->get_pointer<User>(id);
            if (user) {
                return *user;
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to get user by id: " + std::string(e.what()));
        }
    }
    
    std::optional<User> getUserByName(const std::string& name) {
        try {
            auto users = storage_->get_all<User>(sqlite_orm::where(sqlite_orm::like(&User::name, name)));
            if (!users.empty()) {
                return users.front();
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to get user by name: " + std::string(e.what()));
        }
    }
    
    // MessageMemory相关操作
    int insertMessageMemory(const MessageMemory& memory) {
        try {
            return storage_->insert(memory);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to insert message memory: " + std::string(e.what()));
        }
    }
    
    std::vector<MessageMemory> getMessageMemoriesByUserId(int user_id, int limit = 10) {
        try {
            return storage_->get_all<MessageMemory>(
                sqlite_orm::where(sqlite_orm::c(&MessageMemory::user_id) == user_id),
                sqlite_orm::order_by(&MessageMemory::id).desc(),
                sqlite_orm::limit(limit)
            );
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to get message memories: " + std::string(e.what()));
        }
    }
    
    // MultimodalMemory相关操作
    int insertMultimodalMemory(const MultimodalMemory& data) {
        try {
            return storage_->insert(data);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to insert multimodal memory: " + std::string(e.what()));
        }
    }
    
    std::vector<MultimodalMemory> getMultimodalMemoriesByUserId(int user_id, int limit = 10) {
        try {
            return storage_->get_all<MultimodalMemory>(
                sqlite_orm::where(sqlite_orm::c(&MultimodalMemory::user_id) == user_id),
                sqlite_orm::order_by(&MultimodalMemory::id).desc(),
                sqlite_orm::limit(limit)
            );
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to get multimodal memories: " + std::string(e.what()));
        }
    }
    
    bool deleteMultimodalMemory(int id) {
        try {
            storage_->remove<MultimodalMemory>(id);
            return true;
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to delete multimodal memory: " + std::string(e.what()));
        }
    }
    
    // Session相关操作
    int insertSession(const Session& session) {
        try {
            return storage_->insert(session);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to insert session: " + std::string(e.what()));
        }
    }
    
    std::optional<Session> getSessionById(int id) {
        try {
            auto session = storage_->get_pointer<Session>(id);
            if (session) {
                return *session;
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to get session by id: " + std::string(e.what()));
        }
    }
    
    std::vector<Session> getSessionsByUserId(int user_id, int limit = 10) {
        try {
            return storage_->get_all<Session>(
                sqlite_orm::where(sqlite_orm::c(&Session::user_id) == user_id),
                sqlite_orm::limit(limit),
                sqlite_orm::order_by(&Session::id).desc()
            );
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to get sessions: " + std::string(e.what()));
        }
    }
    
    sqlite3* getDatabase() {
        // 返回StorageManager中的数据库连接
        return db_;
    }
    
    // 事务支持
    bool transaction(std::function<bool()> operation) {
        try {
            storage_->begin_transaction();
            bool result = operation();
            if (result) {
                storage_->commit();
            } else {
                storage_->rollback();
            }
            return result;
        } catch (const std::exception& e) {
            storage_->rollback();
            throw std::runtime_error("Transaction failed: " + std::string(e.what()));
        }
    }
    
    // 细粒度事务控制
    void beginTransaction() {
        try {
            storage_->begin_transaction();
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to begin transaction: " + std::string(e.what()));
        }
    }
    
    void commit() {
        try {
            storage_->commit();
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to commit transaction: " + std::string(e.what()));
        }
    }
    
    void rollback() {
        try {
            storage_->rollback();
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to rollback transaction: " + std::string(e.what()));
        }
    }
    
    Storage& getStorage() {
        return *storage_;
    }
};

#endif // ORMWRAPPER_H
