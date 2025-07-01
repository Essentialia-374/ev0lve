#ifndef UTIL_CIRCULAR_BUFFER_H
#define UTIL_CIRCULAR_BUFFER_H

#include <algorithm>
#include <cstdint>
#include <functional>
#include <vector>
#include <cassert>

namespace util
{

    /**
     * @brief  Generic fixed–capacity circular (ring) buffer.
     *
     *  * `current` always points to the logical **back** (oldest element).
     *  * index `0` (via `operator[]`, `begin()` etc.) is the logical **front**
     *    (newest / last pushed element).
     *
     *  The class purposely offers only the operations actually used in the
     *  ev0lve‑fixed code‑base; do **not** add dynamic re‑allocation or capacity
     *  growth here – this would violate real‑time guarantees in several engine
     *  subsystems.
     */
    template <class T, std::size_t N = 0>
    struct circular_buffer
    {
        /* --------------------------------------------------------------------- */
        /*                              ITERATORS                                */
        /* --------------------------------------------------------------------- */
        struct circular_iterator
        {
            circular_buffer* buf = nullptr;
            std::size_t      pos = 0;             // 0 … buf->count

            circular_iterator() = default;
            circular_iterator(circular_buffer* b, std::size_t p) : buf{ b }, pos{ p } {}

            /*  Forward: newest → oldest  */
            circular_iterator& operator++()
            {
                if (buf && ++pos >= buf->count) { buf = nullptr; pos = 0; }
                return *this;
            }

            T* operator->() const { return &(**this); }
            T& operator*()  const { return (*buf)[pos]; }
            bool operator==(const circular_iterator& o) const
            {
                return buf == o.buf && pos == o.pos;
            }
            bool operator!=(const circular_iterator& o) const
            {
                return !(*this == o);
            }
        };

        struct circular_reverse_iterator
        {
            circular_buffer* buf = nullptr;
            std::size_t      pos = 0;             // 0 … buf->count

            circular_reverse_iterator() = default;
            circular_reverse_iterator(circular_buffer* b, std::size_t p) : buf{ b }, pos{ p } {}

            /*  Forward: oldest → newest  */
            circular_reverse_iterator& operator++()
            {
                if (buf && ++pos >= buf->count) { buf = nullptr; pos = 0; }
                return *this;
            }

            T* operator->() const { return &(**this); }
            T& operator*()  const { return (*buf)[buf->count - 1 - pos]; }
            bool operator==(const circular_reverse_iterator& o) const
            {
                return buf == o.buf && pos == o.pos;
            }
            bool operator!=(const circular_reverse_iterator& o) const
            {
                return !(*this == o);
            }
        };

        /* --------------------------------------------------------------------- */
        /*                            CONSTRUCTION                               */
        /* --------------------------------------------------------------------- */
        circular_buffer()
        {
            if ((max_size = N) > 0)
                elem.resize(max_size);
        }

        explicit circular_buffer(std::size_t sz) : max_size(sz)
        {
            elem.resize(max_size);
        }

        /* rule‑of‑five                                                            */
        circular_buffer(const circular_buffer&) = default;
        circular_buffer(circular_buffer&& other) noexcept = default;
        circular_buffer& operator=(const circular_buffer&) = default;

        circular_buffer& operator=(circular_buffer&& other) noexcept
        {
            elem = std::move(other.elem);
            max_size = other.max_size;
            current = other.current;
            count = other.count;

            other.max_size = other.current = other.count = 0;
            return *this;
        }

        ~circular_buffer() = default;

        /* --------------------------------------------------------------------- */
        /*                          ELEMENT ACCESSORS                            */
        /* --------------------------------------------------------------------- */
        [[nodiscard]] inline T& operator[](std::size_t idx)
        {
            assert(idx < count && "index out of range");
            return elem[(current + (count - idx - 1)) % max_size];
        }

        [[nodiscard]] inline const T& operator[](std::size_t idx) const
        {
            assert(idx < count && "index out of range");
            return elem[(current + (count - idx - 1)) % max_size];
        }

        [[nodiscard]] inline std::size_t size()      const { return count; }
        [[nodiscard]] inline bool        empty()     const { return count == 0; }
        [[nodiscard]] inline bool        exhausted() const { return count >= max_size; }

        [[nodiscard]] inline T& back() { return elem[current % max_size]; }
        [[nodiscard]] inline T& front() { return elem[(current + count - 1) % max_size]; }
        [[nodiscard]] inline const T& back()  const { return elem[current % max_size]; }
        [[nodiscard]] inline const T& front() const { return elem[(current + count - 1) % max_size]; }

        /* --------------------------------------------------------------------- */
        /*                          MUTATING OPERATIONS                          */
        /* --------------------------------------------------------------------- */
        inline void clear() { current = count = 0; }

        /* keep the newest entry, discard everything else */
        inline void clear_all_but_first()
        {
            if (count > 1)
            {
                current = (current + count - 1) % max_size;
                count = 1;
            }
        }

        /* remove newest */
        inline void pop_front()
        {
            if (count) --count;
        }

        /* remove oldest */
        inline void pop_back()
        {
            if (!count) return;
            current = (current + 1) % max_size;
            --count;
        }

        /* add a new (default‑constructed) element, return pointer or nullptr if full */
        [[nodiscard]] inline T* push_front()
        {
            if (count >= max_size) return nullptr;

            const std::size_t idx = (current + count) % max_size;
            ++count;
            return &elem[idx];
        }

        /* reserve *capacity* (capacity *changes*) – buffer content is discarded  */
        inline void reserve(std::size_t new_size)
        {
            if (new_size == max_size) return;

            elem.resize(new_size);
            max_size = new_size;
            current = count = 0;
        }

        /* resize and mark the whole buffer as filled (used only in a few places) */
        inline void resize(std::size_t new_size)
        {
            elem.resize(new_size);
            max_size = count = new_size;
            current = 0;
        }

        /* stable in‑place sort (front → back)                                    */
        inline void sort(const std::function<bool(const T&, const T&)>& pred)
        {
            if (count <= 1) return;

            current %= max_size;

            /* unwrap if data are split across the physical end of the vector     */
            if (current + count > max_size)
            {
                std::rotate(elem.begin(),
                    elem.begin() + current,
                    elem.begin() + max_size);
                current = 0;
            }

            auto first = elem.begin() + current;
            std::sort(first, first + count, pred);
        }

        /* --------------------------------------------------------------------- */
        /*                               ITERATION                               */
        /* --------------------------------------------------------------------- */
        [[nodiscard]] inline circular_iterator begin()
        {
            return empty() ? end() : circular_iterator{ this, 0 };
        }
        [[nodiscard]] inline circular_iterator end() { return circular_iterator{}; }

        [[nodiscard]] inline circular_reverse_iterator rbegin()
        {
            return empty() ? rend() : circular_reverse_iterator{ this, 0 };
        }
        [[nodiscard]] inline circular_reverse_iterator rend() { return circular_reverse_iterator{}; }

        /* --------------------------------------------------------------------- */
        /*                              DATA MEMBERS                             */
        /* --------------------------------------------------------------------- */
        std::vector<T> elem;            ///< underlying storage
        std::size_t    max_size = 0;    ///< physical capacity (== elem.size())
        std::size_t    current = 0;    ///< index of logical *back*  (oldest element)
        std::size_t    count = 0;    ///< number of valid elements
    };

} // namespace util
#endif /* UTIL_CIRCULAR_BUFFER_H */
