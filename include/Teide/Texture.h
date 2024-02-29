
#pragma once

#include "GeoLib/Vector.h"
#include "Teide/AbstractBase.h"
#include "Teide/BasicTypes.h"
#include "Teide/TextureData.h"

namespace Teide
{

class RefCounter : AbstractBase
{
public:
    virtual void AddRef(uint64 index) = 0;
    virtual void DecRef(uint64 index) = 0;
};

class Texture final
{
public:
    explicit Texture(uint64 index, RefCounter& owner, TextureData data) : m_index{index}, m_owner{&owner}, m_props{data}
    {}

    Texture(const Texture& other) : m_index{other.m_index}, m_owner{other.m_owner}, m_props{other.m_props}
    {
        m_owner->AddRef(m_index);
    }

    Texture(Texture&& other) : m_index{other.m_index}, m_owner{other.m_owner}, m_props{std::move(other.m_props)}
    {
        other.m_owner = nullptr;
    }

    Texture& operator=(const Texture& other)
    {
        m_index = other.m_index;
        m_owner = other.m_owner;
        m_props = other.m_props;
        m_owner->AddRef(m_index);
        return *this;
    }

    Texture& operator=(Texture&& other)
    {
        m_index = other.m_index;
        m_owner = other.m_owner;
        m_props = std::move(other.m_props);
        other.m_owner = nullptr;
        return *this;
    }

    ~Texture()
    {
        if (m_owner)
        {
            m_owner->DecRef(m_index);
        }
    }

    Geo::Size2i GetSize() const { return m_props.size; }
    Format GetFormat() const { return m_props.format; }
    uint32 GetMipLevelCount() const { return m_props.mipLevelCount; }
    uint32 GetSampleCount() const { return m_props.sampleCount; }

    explicit operator uint64() const { return m_index; }

private:
    uint64 m_index;
    RefCounter* m_owner;
    TextureData m_props;
};

} // namespace Teide
