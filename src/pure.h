#ifndef PURE_H
#define PURE_H

template<class R, class T>
struct pure_op {
        [[no_unique_address]] R recvr;
        T value;

        auto start(){
                recvr.set_value(value);
        }
};

template<class T>
struct pure_sender {
        using value_t = std::tuple<T>;
        T value;

        template<class R>
        auto connect(R recvr){
                return pure_op{recvr, value};
        }
};

auto pure = [](auto value){
        return pure_sender{value};
};

template<class R, class A, class B>
struct pure2_op {
        [[no_unique_address]] R recvr;
        A a;
        B b;

        auto start(){
                recvr.set_value(a, b);
        }
};

template<class A, class B>
struct pure2_sender {
        using value_t = std::tuple<A, B>;
        A a;
        B b;

        template<class R>
        auto connect(R recvr){
                return pure2_op{recvr, a, b};
        }
};

auto pure2 = [](auto a, auto b){
        return pure2_sender{a, b};
};

#endif//PURE_H