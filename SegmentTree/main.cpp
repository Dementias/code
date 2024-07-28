#include "UnefficientSegmentTree.h"
#include <vector>
#include <iostream>

namespace
{
    int sGetSum( const std::vector<int>& array, const std::pair<int, int>& segment )
    {
        auto [lb, rb] = segment;
        if (lb >= rb || rb <= 0 || lb >= array.size() )
        {
            return 0;
        }

        int sum = 0;
        rb = std::min( rb, (int)array.size() );
        for ( auto i = std::max( 0, lb ); i < rb; ++i )
        {
            sum += array[i];
        }
        return sum;
    }
}

int main()
{
    std::vector<int> inputArray = { 1, 3, 2, 4, 7, 6, 5, 8, 10, 9, 11, 22, 12, 9, 9, 9, 1, 1, 3, 3, 3, 3};
    std::vector<std::pair<int, int>> segments = {{0, 0}, {0, 30}, {0, -10}, {2, -10}, {0, 5}, {7, 15}};
    UnefficientSegmentTree segTree( 0, inputArray.size() );
    
    for ( auto i = 0; i < inputArray.size(); ++i )
    {
        segTree.add( i, inputArray[i] );
    }

    auto n = 0;
    for ( auto& segment : segments )
    {
        auto segResult = segTree.getSum( segment.first, segment.second );
        auto expectedResult = sGetSum( inputArray, segment );
        std::string passed = segResult == expectedResult ? "PASSED" : "FAILED";
        std::cout << n++ << "th TEST " << passed;
        if ( passed == "FAILED" )
        {
            std::cout << " Segment Tree Result: " << segResult << ". Expected Result: "<< expectedResult;
        }
        std::cout << std::endl;
    }
}