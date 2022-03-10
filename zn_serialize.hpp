/*
 * 基于C++11的字节序列化工具
 * gitee地址: https://gitee.com/tony_zn/zn-serialize
 * github地址: https://github.com/tony-zn/ZnSerialize
 * 作者: tony.zn
*/
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
#include <tuple>
#include <memory>

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
        typedef Struct ZnSerialize;
        virtual void serialize(ZnSerializeBuffer& buffer) const = 0;
        virtual void deserialize(const ZnSerializeBuffer& buffer) = 0;
        virtual const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end) = 0;
    };

    template<typename child_t, typename parent_t, typename...args>
    struct Parent : public parent_t, public Parent<Parent<child_t, parent_t, args...>, args...>
    {
        typedef Parent<child_t, parent_t, args...>   parent_pack_t;
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

    template<typename t> inline t* get_zn_struct(double);
    template<typename t> inline typename t::ZnSerialize* get_zn_struct(int);
    template<typename t> struct GetZnStructPtr { typedef decltype(get_zn_struct<t>(0)) Ptr;};

    template<typename t>
    inline void default_serialize(ZnSerializeBuffer& out, const t& v, t* p)
    {
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&v), reinterpret_cast<const uint8_t*>(&v) + sizeof(v));
    }

    template<typename t>
    inline void default_serialize(ZnSerializeBuffer& out, const t& v, Struct* p)
    {
        p->serialize(out);
    }

    template<typename t>
    inline const uint8_t* default_deserialize(const uint8_t* begin, const uint8_t* end, t& v, t* p)
    {
        if (begin + sizeof(v) > end)
            throw Exception("deserialize value failed, out of memery");
        v = *reinterpret_cast<const t*>(begin);
        return begin + sizeof(v);
    }

    template<typename t>
    inline const uint8_t* default_deserialize(const uint8_t* begin, const uint8_t* end, t& v, Struct* p)
    {
        return p->deserialize(begin, end);
    }

    template<typename t>
    inline void serialize(ZnSerializeBuffer& out, const t& v)
    {
        default_serialize(out, v, static_cast<typename GetZnStructPtr<t>::Ptr>(const_cast<t*>(&v)));
    }

    template<typename t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, t& v)
    {
        return default_deserialize(begin, end, v, static_cast<typename GetZnStructPtr<t>::Ptr>(const_cast<t*>(&v)));
    }

    template<typename t>
    inline void serialize(ZnSerializeBuffer& out, const std::shared_ptr<t>& v)
    {
        serialize(out, *v);
    }

    template<typename t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::shared_ptr<t>& v)
    {
        if (!v)
            v = std::make_shared<t>();
        return deserialize(begin, end, *v);
    }

    template<>
    inline void serialize(ZnSerializeBuffer& out, const std::string& v)
    {
        uint32_t size = static_cast<uint32_t>(v.size());
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(v.data()), reinterpret_cast<const uint8_t*>(v.data()) + size);
    }

    template<>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::string& v)
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
    inline void serialize(ZnSerializeBuffer& out, const std::wstring& v)
    {
        uint32_t size = static_cast<uint32_t>(v.size() * sizeof(wchar_t));
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(v.data()), reinterpret_cast<const uint8_t*>(v.data()) + size);
    }

    template<>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::wstring& v)
    {
        if (begin + sizeof(uint32_t) > end)
            throw Exception("deserialize string failed, out of memery");
        auto p = begin;
        uint32_t size = *reinterpret_cast<const uint32_t*>(p);
        p += sizeof(uint32_t);
        if (p + size > end)
            throw Exception("deserialize string failed, out of memery");
        v.assign(reinterpret_cast<const wchar_t*>(p), reinterpret_cast<const wchar_t*>(p + size));
        return p + size;
    }

    template<size_t i, typename...t>
    struct ForeachTuple
    {
        void serialize(ZnSerializeBuffer& out, const std::tuple<t...>& tuple)
        {
            ForeachTuple<i-1, t...>().serialize(out, tuple);
            zn_serialize::serialize(out, std::get<i>(tuple));
        }
        const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::tuple<t...>& tuple)
        {
            auto p = ForeachTuple<i-1, t...>().deserialize(begin, end, tuple);
            return zn_serialize::deserialize(p, end, std::get<i>(tuple));
        }
    };

    template<typename...t>
    struct ForeachTuple<0, t...>
    {
        void serialize(ZnSerializeBuffer& out, const std::tuple<t...>& tuple)
        {
            zn_serialize::serialize(out, std::get<0>(tuple));
        }
        const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::tuple<t...>& tuple)
        {
            return zn_serialize::deserialize(begin, end, std::get<0>(tuple));
        }
    };

    template<typename...t>
    inline void serialize(ZnSerializeBuffer& out, const std::tuple<t...>& v)
    {
        ForeachTuple<sizeof...(t)-1, t...>().serialize(out, v);
    }

    template<typename...t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::tuple<t...>& v)
    {
        return ForeachTuple<sizeof...(t)-1, t...>().deserialize(begin, end, v);
    }

    template<typename t, uint32_t s>
    inline void serialize(ZnSerializeBuffer& out, const t(&v)[s])
    {
        for (uint32_t i = 0; i < s; ++i)
            serialize(out, v[i]);
    }

    template<typename t, uint32_t s>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, t(&v)[s])
    {
        auto p = begin;
        for (uint32_t i = 0; i < s; ++i)
            p = deserialize(p, end, v[i]);
        return p;
    }


    template<>
    inline void serialize(ZnSerializeBuffer& out, const Struct& v)
    {
        const_cast<Struct&>(v).serialize(out);
    }

    template<>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, Struct& v)
    {
        return v.deserialize(begin, end);
    }

    template<typename t>
    inline void serialize_container(ZnSerializeBuffer& out, const t& v)
    {
        uint32_t size = static_cast<uint32_t>(v.size());
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        for (const auto& i : v)
            serialize(out, i);
    }

    template<typename t>
    inline void serialize_map(ZnSerializeBuffer& out, const t& v)
    {
        uint32_t size = static_cast<uint32_t>(v.size());
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&size), reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
        for (const auto& i : v)
        {
            serialize(out, i.first);
            serialize(out, i.second);
        }
    }

    template<typename t>
    inline void serialize(ZnSerializeBuffer& out, const std::vector<t>& v) { serialize_container(out, v); }
    template<typename t>
    inline void serialize(ZnSerializeBuffer& out, const std::deque<t>& v) { serialize_container(out, v); }
    template<typename t>
    inline void serialize(ZnSerializeBuffer& out, const std::list<t>& v) { serialize_container(out, v); }
    template<typename t>
    inline void serialize(ZnSerializeBuffer& out, const std::set<t>& v) { serialize_container(out, v); }
    template<typename t>
    inline void serialize(ZnSerializeBuffer& out, const std::multiset<t>& v) { serialize_container(out, v); }
    template<typename t>
    inline void serialize(ZnSerializeBuffer& out, const std::stack<t>& v) { throw Exception("serialize failed, not allowed on statck"); }
    template<typename t>
    inline void serialize(ZnSerializeBuffer& out, const std::queue<t>& v) { throw Exception("serialize failed, not allowed on queue"); }
    template<typename t>
    inline void serialize(ZnSerializeBuffer& out, const std::priority_queue<t>& v) { throw Exception("serialize failed, not allowed on priority_queue"); }
    template<typename k, typename t>
    inline void serialize(ZnSerializeBuffer& out, const std::map<k, t>& v) { serialize_map(out, v); }
    template<typename k, typename t>
    inline void serialize(ZnSerializeBuffer& out, const std::multimap<k, t>& v) { serialize_map(out, v); }


    template<typename t>
    inline const uint8_t* deserialize_container(const uint8_t* begin, const uint8_t* end, t& v)
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
    inline const uint8_t* deserialize_set(const uint8_t* begin, const uint8_t* end, t& v)
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
    inline const uint8_t* deserialize_map(const uint8_t* begin, const uint8_t* end, t& v)
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
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::vector<t>& v) { return deserialize_container(begin, end, v); }
    template<typename t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::deque<t>& v) { return deserialize_container(begin, end, v); }
    template<typename t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::list<t>& v) { return deserialize_container(begin, end, v); }
    template<typename t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::set<t>& v) { return deserialize_set(begin, end, v); }
    template<typename t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::multiset<t>& v) { return deserialize_set(begin, end, v); }
    template<typename t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::stack<t>& v) { throw Exception("deserialize failed, not allowed on statck"); }
    template<typename t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::queue<t>& v) { throw Exception("deserialize failed, not allowed on queue"); }
    template<typename t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::priority_queue<t>& v) { throw Exception("deserialize failed, not allowed on priority_queue"); }
    template<typename k, typename t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::map<k, t>& v) { return deserialize_map(begin, end, v); }
    template<typename k, typename t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, std::multimap<k, t>& v) { return deserialize_map(begin, end, v); }

    template<typename t, typename...args_t>
    inline void serialize(ZnSerializeBuffer& out, const t& v, const args_t&...args)
    {
        serialize(out, v);
        serialize(out, args...);
    }

    template<typename t, typename...args_t>
    inline const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end, t& v, args_t&...args)
    {
        return deserialize(deserialize(begin, end, v), end, args...);
    }

    template<typename t, typename...args_t>
    inline const uint8_t* deserialize(const ZnSerializeBuffer& in, t& v, args_t&...args)
    {
        auto end = in.data() + in.size();
        return deserialize(deserialize(in.data(), end, v), end, args...);
    }

    template<typename t, typename...parents_t>
    struct AutoAdaptBase : public Parent<t, parents_t...>
    {
    protected:
        template<typename ...args_t>
        void auto_adapt_serialize(t* child, ZnSerializeBuffer& buffer, const args_t&...args)
        {
            Parent<t, parents_t...>::parent_pack_t::parent_serialize(child, buffer);
            zn_serialize::serialize(buffer, args...);
        }
        void auto_adapt_serialize(t* child, ZnSerializeBuffer& buffer)
        {
            Parent<t, parents_t...>::parent_pack_t::parent_serialize(child, buffer);
        }
        template<typename ...args_t>
        const uint8_t* auto_adapt_deserialize(t* child, const uint8_t* begin, const uint8_t* end, args_t&...args)
        {
            auto p = Parent<t, parents_t...>::parent_pack_t::parent_deserialize(child, begin, end);
            return zn_serialize::deserialize(p, end, args...);
        }
        const uint8_t* auto_adapt_deserialize(t* child, const uint8_t* begin, const uint8_t* end)
        {
            return Parent<t, parents_t...>::parent_pack_t::parent_deserialize(child, begin, end);
        }
    };

    template<typename t>
    struct AutoAdaptBase<t> : public Struct
    {
    protected:
        template<typename...args_t>
        void auto_adapt_serialize(t* child, ZnSerializeBuffer& buffer, const args_t&...args)
        {
            zn_serialize::serialize(buffer, args...);
        }
        template<typename...args_t>
        const uint8_t* auto_adapt_deserialize(t* child, const uint8_t* begin, const uint8_t* end, args_t&...args)
        {
            return zn_serialize::deserialize(begin, end, args...);
        }
    };

    template<typename first_member_t, typename...other_members_t>
    struct AssignmentMembers
    {
        template<typename first_value_t, typename...other_values_t>
        void operator()(first_member_t& first_member, other_members_t&...other_members
            , const first_value_t& first_value, const other_values_t&...other_values)
        {
            first_member = first_value;
            AssignmentMembers<other_members_t...>()(other_members..., other_values...);
        }
        template<typename first_value_t>
        void operator()(first_member_t& first_member, other_members_t&...other_members, const first_value_t& first_value)
        {
            first_member = first_value;
        }
    };

    template<typename first_member_t>
    struct AssignmentMembers<first_member_t>
    {
        template<typename first_value_t, typename...other_values_t>
        void operator()(first_member_t& first_member, const first_value_t& first_value, const other_values_t&...other_values)
        {
            first_member = first_value;
        }
    };

    template<>
    struct AssignmentMembers<void>
    {
        template<typename...values_t>
        void operator()(const values_t&...values)
        {}
    };

    template<typename...members_t>
    inline AssignmentMembers<members_t...> get_assignment_members_type(members_t&...);

    inline AssignmentMembers<void> get_assignment_members_type();

    template<typename t> inline t* cancel_const(const t* p){ return const_cast<t*>(p);}
};

// 通过 ZN_VA_OPT_SUPPORTED 宏来自动判断是否支持 __VA_OPT__
#define ZN_PP_THIRD_ARG(a,b,c,...) c
#define ZN_VA_OPT_SUPPORTED_I(...) ZN_PP_THIRD_ARG(__VA_OPT__(,),1,0,)
#define ZN_VA_OPT_SUPPORTED ZN_VA_OPT_SUPPORTED_I(?)

#if ZN_VA_OPT_SUPPORTED == 0

#define ZN_STRUCT(name,...)     struct name : public zn_serialize::AutoAdaptBase<name, ##__VA_ARGS__>
#define ZN_SERIALIZE(...)       void serialize(ZnSerializeBuffer& buffer) const { zn_serialize::cancel_const(this)->auto_adapt_serialize(zn_serialize::cancel_const(this), buffer, ##__VA_ARGS__); }\
                                void deserialize(const ZnSerializeBuffer& buffer){ deserialize(buffer.data(), buffer.data() + buffer.size()); }\
                                const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end){ return this->auto_adapt_deserialize(this, begin, end, ##__VA_ARGS__); }\
                                template<typename...values_t> void znset(const values_t&...other_values){decltype(zn_serialize::get_assignment_members_type(__VA_ARGS__))()(__VA_ARGS__,other_values...);}

#else

#define ZN_STRUCT(name,...)     struct name : public zn_serialize::AutoAdaptBase<name __VA_OPT__(,) __VA_ARGS__>
#define ZN_SERIALIZE(...)       void serialize(ZnSerializeBuffer& buffer) const { zn_serialize::cancel_const(this)->auto_adapt_serialize(zn_serialize::cancel_const(this), buffer __VA_OPT__(,) __VA_ARGS__); }\
                                void deserialize(const ZnSerializeBuffer& buffer){ deserialize(buffer.data(), buffer.data() + buffer.size()); }\
                                const uint8_t* deserialize(const uint8_t* begin, const uint8_t* end){ return this->auto_adapt_deserialize(this, begin, end __VA_OPT__(,) __VA_ARGS__); }\
                                template<typename...values_t> void znset(const values_t&...other_values){decltype(zn_serialize::get_assignment_members_type(__VA_ARGS__))()(__VA_ARGS__ __VA_OPT__(,) other_values...);}

#endif
