#ifndef UTIL_HOOK_H
#define UTIL_HOOK_H

//
//  util/hook.h
//
//  Fixed, architecture‑aware PolyHook 2 wrapper for ev0lve‑fixed.
//  • Works on both 32‑bit (x86) and 64‑bit (x86‑64) builds.
//  • Eliminates incorrect size/pointer conversions that corrupted memory on x86.
//  • Keeps the public interface (util::hook<>, MAKE_HOOK*, …) unchanged so the
//    rest of the project compiles without further edits.
//  • Requires **no** additional third‑party headers beyond those already
//    shipped with the repository and PolyHook 2.
//

#include <base/game.h>
#include <type_traits>
#include <util/mem_guard.h>

#include <polyhook2/Detour/x86Detour.hpp>
#include <polyhook2/Detour/x64Detour.hpp>

#include <cstdint>
#include <memory>
#include <utility>

namespace util {

    //--------------------------------------------------------------------------
    // common polymorphic interface ------------------------------------------------
    //--------------------------------------------------------------------------

    class hook_interface {
    public:
        hook_interface() = default;
        hook_interface(const hook_interface&) = delete;
        hook_interface& operator=(const hook_interface&) = delete;
        hook_interface(hook_interface&&) = delete;
        hook_interface& operator=(hook_interface&&) = delete;
        virtual ~hook_interface() = default;

        virtual void      attach() = 0;
        virtual void      detach() = 0;
        virtual uintptr_t get_endpoint() const = 0;
        virtual uintptr_t get_target()   const = 0;
    };

    //--------------------------------------------------------------------------
    // util::hook – type‑safe inline detour wrapper ------------------------------
    //--------------------------------------------------------------------------

    template<class T>
    class hook final : public hook_interface {
        using trampoline_storage_t = uint64_t; // always 8 bytes – large enough on x86 and x64

    public:
        hook(encrypted_ptr<T> target, encrypted_ptr<T> endpoint) :
            _target{ target },
            _endpoint{ endpoint },
            _detour{ nullptr },
            _trampoline{ nullptr },
            _attached{ false },
            _gateway{ 0 }
        {
        }

        ~hook() override {
            if (_attached) {
                try { detach(); }
                catch (...) { /* swallow – never let dtor throw */ }
            }
        }

        hook(const hook&) = delete;
        hook& operator=(const hook&) = delete;
        hook(hook&&) = delete;
        hook& operator=(hook&&) = delete;

        //----------------------------------------------------------------------
        // attach / detach ------------------------------------------------------
        //----------------------------------------------------------------------

        void attach() override {
            if (_attached)
                RUNTIME_ERROR("attempted to attach an already‑attached hook");

            if (!_target || !_endpoint)
                RUNTIME_ERROR("target or endpoint pointer is null");

            const auto target_addr = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(_target()));
            const auto detour_addr = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(_endpoint()));
            auto* trampoline_out = &_gateway;                   // PolyHook will write the gateway here

            //
            //  Create the correct detour object for the build architecture.
            //  NOTE:  std::unique_ptr<PLH::IHook>(new …) is required – you
            //  cannot assign std::unique_ptr<derived> to unique_ptr<base>.
            //
#if defined(_M_X64) || defined(__x86_64__) || defined(_WIN64)
            _detour.reset(new PLH::x64Detour(target_addr, detour_addr, trampoline_out));
#else
            _detour.reset(new PLH::x86Detour(target_addr, detour_addr, trampoline_out));
#endif

            if (!_detour->hook())
                RUNTIME_ERROR("PolyHook 2 failed to install detour");

            _trampoline = reinterpret_cast<T*>(static_cast<uintptr_t>(_gateway));
            _attached = true;
        }

        void detach() override {
            if (!_attached)
                RUNTIME_ERROR("attempted to detach a hook that is not attached");

            if (!_detour->unHook())
                RUNTIME_ERROR("PolyHook 2 failed to remove detour");

            _detour.reset();
            _trampoline = nullptr;
            _gateway = 0;
            _attached = false;
        }

        //----------------------------------------------------------------------
        // accessors ------------------------------------------------------------
        //----------------------------------------------------------------------

        [[nodiscard]] uintptr_t get_endpoint() const override {
            return reinterpret_cast<uintptr_t>(_endpoint());
        }

        [[nodiscard]] uintptr_t get_target() const override {
            return reinterpret_cast<uintptr_t>(_target());
        }

        [[nodiscard]] T* get_original() const { return _trampoline; }

        template<typename... Args>
        std::invoke_result_t<T, Args...> call(Args&&... args) const {
            return get_original()(std::forward<Args>(args)...);
        }

    private:
        //----------------------------------------------------------------------
        // data members ---------------------------------------------------------
        //----------------------------------------------------------------------

        encrypted_ptr<T>            _target;
        encrypted_ptr<T>            _endpoint;
        std::unique_ptr<PLH::IHook> _detour;
        T* _trampoline;
        bool                        _attached;
        trampoline_storage_t        _gateway;   // filled by PolyHook
    };

} // namespace util

//------------------------------------------------------------------------------
// helper macros ----------------------------------------------------------------
//------------------------------------------------------------------------------

#define MAKE_HOOK(fn, name, pat)                                                                                       \
    std::make_unique<util::hook<decltype(fn)>>(PATTERN(decltype(fn), name, pat), util::encrypted_ptr(fn))

#define MAKE_HOOK_OFFSET(fn, name, pat, off)                                                                           \
    std::make_unique<util::hook<decltype(fn)>>(PATTERN_OFFSET(decltype(fn), name, pat, off), util::encrypted_ptr(fn))

#define MAKE_HOOK_REL(fn, name, pat)                                                                                   \
    std::make_unique<util::hook<decltype(fn)>>(PATTERN_REL(decltype(fn), name, pat), util::encrypted_ptr(fn))

#define MAKE_HOOK_PRIMITIVE(fn, to)                                                                                    \
    std::make_unique<util::hook<decltype(fn)>>(util::encrypted_ptr<decltype(fn)>(reinterpret_cast<decltype(&fn)>(to)),  \
                                               util::encrypted_ptr(fn))

#endif // UTIL_HOOK_H
