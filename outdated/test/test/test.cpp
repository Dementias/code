#include <iostream>

void overloaded( int& arg )
{
    std::cout << "by lvalue\n";
}
void overloaded( int&& arg )
{
    std::cout << "by rvalue\n";
}

template<typename T>
void forwarding( T&& arg )
{
    std::cout << "via std::forward: ";
    overloaded( std::forward<T>( arg ) );
    std::cout << "via std::move: ";
    overloaded( std::move( arg ) ); // conceptually this would invalidate arg
    std::cout << "by simple passing: ";
    overloaded( arg );
}

int main()
{
    std::cout << "initial caller passes rvalue:\n";
    forwarding( 5 );
    std::cout << "initial caller passes lvalue:\n";
    int x = 5;
    forwarding( x );
}

