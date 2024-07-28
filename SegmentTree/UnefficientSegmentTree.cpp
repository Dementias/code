#include "UnefficientSegmentTree.h"
UnefficientSegmentTree::UnefficientSegmentTree( int leftBorder_, int rightBorder_ ) : leftBorder( leftBorder_ ), rightBorder( rightBorder_ )
    {
        if ( rightBorder - leftBorder > 1 )
        {
            auto middle = ( leftBorder + rightBorder ) / 2;
            leftSegment = new UnefficientSegmentTree ( leftBorder, middle );
            rightSegment = new UnefficientSegmentTree ( middle, rightBorder );
        }
    }
    UnefficientSegmentTree::~UnefficientSegmentTree()
    {
        delete leftSegment;
        delete rightSegment;
    }

    void UnefficientSegmentTree:: add( int k, int value )
    {
        if ( leftBorder <= k && k < rightBorder )
        {
            sum +=value;
            if (leftSegment)
            {
                if (k < leftSegment->rightBorder )
                {
                    leftSegment->add( k, value );
                }
                else
                {
                    rightSegment->add( k, value );
                }
            }
        }
    }

    int UnefficientSegmentTree::getSum( int lb, int rb ) const
    {
        if ( lb >= rb )
        {
            return 0;
        }
        else if ( lb <= leftBorder && rb >= rightBorder )
        {
            return sum;
        }
        else if ( rb <= leftBorder || lb >= rightBorder )
        {
            return 0;
        }
        return leftSegment->getSum( lb, rb ) + rightSegment->getSum( lb, rb );
    }