#include "sort.hpp"
#include <algorithm>
#include <assert.h>

std::vector<int> data { 6, 19, 3, 28, 777, 4, 98, 10, 8 };

int main(int argc, char* argv[])
{
    std::vector<int> store;

    std::vector<int> sorted = data;
    std::sort(sorted.begin(), sorted.end());

    BubbleSort().sort_it("BubbleSort", data, store); assert( store == sorted );
    BubbleSort2().sort_it("BubbleSort2", data, store); assert( store == sorted );

    InsertSort().sort_it("InsertSort", data, store); assert( store == sorted );
    InsertSort2().sort_it("InsertSort2", data, store); assert( store == sorted );

    ShellSort().sort_it("ShellSort", data, store); assert( store == sorted );

    SelectSort().sort_it("SelectSort", data, store); assert( store == sorted );

    HeapSort().sort_it("HeapSort", data, store); assert( store == sorted );

    ShakerSort().sort_it("ShakerSort", data, store); assert( store == sorted );

    QuickSort().sort_it("QuickSort", data, store); assert( store == sorted );

    MergeSort().sort_it("MergeSort", data, store); assert( store == sorted );

    CountingSort().sort_it("CountingSort", data, store); assert( store == sorted );

	return 0;
}
