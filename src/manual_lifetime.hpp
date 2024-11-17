#ifndef MANUAL_LIFETIME_HPP
#define MANUAL_LIFETIME_HPP
    
namespace ex {

    template<typename T>
    struct manual_lifetime {
    
        alignas(T) std::byte storage[sizeof(T)];
    
        manual_lifetime() noexcept {}
    
        template<class Factory>
        manual_lifetime(Factory factory){
            this->construct_from(factory);
        }
    
        ~manual_lifetime() = default;

        // Not copyable/movable
        manual_lifetime(const manual_lifetime&) = delete;
        manual_lifetime(manual_lifetime&&) = delete;
        manual_lifetime& operator=(const manual_lifetime&) = delete;
        manual_lifetime& operator=(manual_lifetime&&) = delete;

        template<typename Factory>
        requires std::invocable<Factory&> && std::same_as<std::invoke_result_t<Factory&>, T>
        T& construct_from(Factory factory) noexcept(std::is_nothrow_invocable_v<Factory&>){
            return *::new (static_cast<void*>(&storage)) T(factory());
        }

        void destroy() noexcept(std::is_nothrow_destructible_v<T>) {
            std::destroy_at(std::launder(reinterpret_cast<T*>(&storage)));
        }

        T& get() & noexcept {
            return *std::launder(reinterpret_cast<T*>(&storage));
        }

    };
    
}//namespace ex

#endif//MANUAL_LIFETIME_HPP
