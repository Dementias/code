#pragma once
// [leftBorder, rightBorder) 
// leftBorder - included
// rightBorder - not included
class UnefficientSegmentTree
{
private:
    int leftBorder, rightBorder;
    int sum = 0;

    UnefficientSegmentTree *leftSegment = nullptr;
    UnefficientSegmentTree *rightSegment = nullptr;

public:
    UnefficientSegmentTree( int leftBorder_, int rightBorder_ );
    ~UnefficientSegmentTree();

    void add( int k, int value );
    int getSum( int lb, int rb ) const;
};