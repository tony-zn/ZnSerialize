#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <stack>
#include <queue>

typedef std::vector<uint8_t> ZnSerializeBuffer;

namespace zn_serialize
{
    struct Exception
    {
    public:
        explicit Exception(const char* message)
            : message_(message)
        {}
        const char* what() const { return message_.c_str(); }
    private:
        std::string message_;
    };
    struct Struct
    {
        virtual void serialize(ZnSerializeBuffer& buffer) = 0;
        virtual void deserialize(const ZnSerializeBuffer& buffer) = 0;
        virtual const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end) = 0;
    };

    template<typename child_t, typename parent_t, typename...args>
    struct Parent : public parent_t, public Parent<Parent<child_t, parent_t, args...>, args...>
    {
        typedef Parent<child_t, parent_t, args...> parent_pack_t;
    protected:
        void parent_serialize(child_t* child, ZnSerializeBuffer& buffer)
        {
            child->parent_t::serialize(buffer);
            Parent<Parent<child_t, parent_t, args...>, args...>::parent_serialize(this, buffer);
        }
        const uint8_t* parent_deserialize(child_t* child, const uint8_t* begin, const uint8_t* end)
        {
            auto p = child->parent_t::deserialize(begin, end);
            return Parent<Parent<child_t, parent_t, args...>, args...>::parent_deserialize(this, p, end);
        }
    };

    template<typename child_t, typename parent_t>
    struct Parent<child_t, parent_t> : public parent_t
    {
        typedef Parent<child_t, parent_t>   parent_pack_t;
    protected:
        void parent_serialize(child_t* child, ZnSerializeBuffer& buffer)
        {
            child->parent_t::serialize(buffer);
        }
        const uint8_t* parent_deserialize(child_t* child, const uint8_t* begin, const uint8_t* end)
        {
            return child->parent_t::deserialize(begin, end);
        }
    };

    template<typename t>
    void serialize(ZnSerializeBuffer& out, const t& v)
    {
        uint32_t size = sizeof(v);
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&v), reinterpret_cast<const uint8_t*>(&v) + size);
    }

    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, t& v)
    {
        if (begin + sizeof(uint32_t) > end)
            throw Exception("deserialize value failed, out of memery");
        auto p = begin;
        uint32_t size = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        if (size != sizeof(v) || p + size > end)
            throw Exception("deserialize value failed, out of memery");
        v = *reinterpret_cast<const t*>(p);
        return p + size;
    }

    template<>
    void serialize(ZnSerializeBuffer& out, const std::string& v)
    {
        uint32_t size = v.size();
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(v.data()), reinterpret_cast<const uint8_t*>(v.data()) + size);
    }

    template<>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::string& v)
    {
        if (begin + sizeof(uint32_t) > end)
            throw Exception("deserialize string failed, out of memery");
        auto p = begin;
        uint32_t size = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        if (p + size > end)
            throw Exception("deserialize string failed, out of memery");
        v.assign(p, p + size);
        return p + size;
    }

    template<>
    void serialize(ZnSerializeBuffer& out, const Struct& v)
    {
        const_cast<Struct&>(v).serialize(out);
    }

    template<>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, Struct& v)
    {
        return v.deserialize(begin, end);
    }

    template<typename t>
    void serialize_container(ZnSerializeBuffer& out, const t& v)
    {
        uint32_t size = v.size();
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        for (const auto& i : v)
            serialize(out, i);
    }

    template<typename t>
    void serialize_map(ZnSerializeBuffer& out, const t& v)
    {
        uint32_t size = v.size();
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        for (const auto& i : v)
        {
            serialize(out, i.first);
            serialize(out, i.second);
        }
    }

    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::vector<t>& v) { serialize_container(out, v); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::deque<t>& v) { serialize_container(out, v); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::list<t>& v) { serialize_container(out, v); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::set<t>& v) { serialize_container(out, v); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::multiset<t>& v) { serialize_container(out, v); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::stack<t>& v) { throw Exception("serialize failed, not allowed on statck"); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::queue<t>& v) { throw Exception("serialize failed, not allowed on queue"); }
    template<typename t>
    void serialize(ZnSerializeBuffer& out, const std::priority_queue<t>& v) { throw Exception("serialize failed, not allowed on priority_queue"); }
    template<typename k, typename t>
    void serialize(ZnSerializeBuffer& out, const std::map<k, t>& v) { serialize_map(out, v); }
    template<typename k, typename t>
    void serialize(ZnSerializeBuffer& out, const std::multimap<k, t>& v) { serialize_map(out, v); }


    template<typename t>
    const uint8_t* deserialize_container(const uint8_t* begin, const uint8_t* end, t& v)
    {
        if (begin + sizeof(uint32_t) > end)
            throw Exception("deserialize container failed, out of memery");
        auto p = begin;
        uint32_t size = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        for (uint32_t i = 0; i < size; ++i)
        {
            typename t::value_type item;
            p = deserialize(p, end, item);
            v.push_back(item);
        }
        return p;
    }

    template<typename t>
    const uint8_t* deserialize_set(const uint8_t* begin, const uint8_t* end, t& v)
    {
        if (begin + sizeof(uint32_t) > end)
            throw Exception("deserialize set failed, out of memery");
        auto p = begin;
        uint32_t size = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        for (uint32_t i = 0; i < size; ++i)
        {
            typename t::value_type item;
            p = deserialize(p, end, item);
            v.insert(item);
        }
        return p;
    }

    template<typename t>
    const uint8_t* deserialize_map(const uint8_t* begin, const uint8_t* end, t& v)
    {
        if (begin + sizeof(uint32_t) > end)
            throw Exception("deserialize map failed, out of memery");
        auto p = begin;
        uint32_t size = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        for (uint32_t i = 0; i < size; ++i)
        {
            typename t::key_type key;
            typename t::value_type::second_type item;
            p = deserialize(p, end, key);
            p = deserialize(p, end, item);
            v.insert(std::make_pair(key, item));
        }
        return p;
    }

    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::vector<t>& v) { return deserialize_container(begin, end, v); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::deque<t>& v) { return deserialize_container(begin, end, v); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::list<t>& v) { return deserialize_container(begin, end, v); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::set<t>& v) { return deserialize_set(begin, end, v); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::multiset<t>& v) { return deserialize_set(begin, end, v); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::stack<t>& v) { throw Exception("deserialize failed, not allowed on statck"); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::queue<t>& v) { throw Exception("deserialize failed, not allowed on queue"); }
    template<typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::priority_queue<t>& v) { throw Exception("deserialize failed, not allowed on priority_queue"); }
    template<typename k, typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::map<k, t>& v) { return deserialize_map(begin, end, v); }
    template<typename k, typename t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::multimap<k, t>& v) { return deserialize_map(begin, end, v); }

    template<typename t, typename...args_t>
    void serialize(ZnSerializeBuffer& out, const t& v, const args_t&...args)
    {
        serialize(out, v);
        serialize(out, args...);
    }

    template<typename t, typename...args_t>
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, t& v, args_t&...args)
    {
        return deserialize(deserialize(begin, end, v), end, args...);
    }

    template<typename t, typename...args_t>
    const uint8_t* deserialize(const ZnSerializeBuffer& in, t& v, args_t&...args)
    {
        auto end = in.data() + in.size();
        return deserialize(deserialize(in.data(), end, v), end, args...);
    }
};

#define ZN_STRUCT(name) struct name : public zn_serialize::Struct

#define ZN_STRUCT_CHILD(name, ...) struct name : public zn_serialize::Parent<name, __VA_ARGS__>

#define ZN_SERIALIZE(...) void serialize(ZnSerializeBuffer& buffer){ zn_serialize::serialize(buffer, __VA_ARGS__); }\
    void deserialize(const ZnSerializeBuffer& buffer){ deserialize(buffer.data(), buffer.data() + buffer.size()); }\
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end){ return zn_serialize::deserialize(begin, end, __VA_ARGS__); }

#define ZN_SERIALIZE_CHILD(...) void serialize(ZnSerializeBuffer& buffer){ parent_pack_t::parent_serialize(this, buffer); zn_serialize::serialize(buffer, __VA_ARGS__); }\
    void deserialize(const ZnSerializeBuffer& buffer){ deserialize(buffer.data(), buffer.data() + buffer.size()); }\
    const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end){ auto p = parent_pack_t::parent_deserialize(this, begin, end); return zn_serialize::deserialize(p, end, __VA_ARGS__); }
