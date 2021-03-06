#ifndef BURST_ALGORITHM_SORTING_DETAIL_COUNTING_SORT_HPP
#define BURST_ALGORITHM_SORTING_DETAIL_COUNTING_SORT_HPP

#include <burst/container/access/cback.hpp>

#include <algorithm>
#include <iterator>
#include <limits>
#include <numeric>
#include <type_traits>
#include <vector>

namespace burst
{
    namespace detail
    {
        template <typename Value, typename Map>
        struct counting_sort_traits
        {
            using image_type = typename std::decay<typename std::result_of<Map(Value)>::type>::type;
            static_assert
            (
                std::is_integral<image_type>::value && std::is_unsigned<image_type>::value,
                "Сортируемые элементы должны быть отображены в целые числа."
            );

            constexpr static const auto value_range = std::numeric_limits<image_type>::max() + 1;
        };

        //!     Собрать счётчики.
        /*!
                Для каждого сортируемого числа подсчитывает количество элементов, строго меньших
            этого числа.
         */
        template <typename ForwardIterator, typename Map, typename Array>
        void collect (ForwardIterator first, ForwardIterator last, Map map, Array & counters)
        {
            std::for_each(first, last,
                [& counters, & map] (const auto & preimage)
                {
                    ++counters[map(preimage) + 1];
                });

            std::partial_sum(std::begin(counters), std::end(counters), std::begin(counters));
        }

        //!     Расставить по местам.
        /*!
                Расставляет сортируемые элементы в выходной диапазон в соответствии с известным
            массивом счётчиков.
         */
        template <typename ForwardIterator, typename RandomAccessIterator, typename Map, typename Array>
        void dispose (ForwardIterator first, ForwardIterator last, RandomAccessIterator result, Map map, Array & counters)
        {
            std::for_each(first, last,
                [& result, & counters, & map] (auto && preimage)
                {
                    auto index = counters[map(preimage)]++;
                    result[index] = std::forward<decltype(preimage)>(preimage);
                });
        }

        template <typename ForwardIterator, typename RandomAccessIterator, typename Map, typename Array>
        void dispose_move (ForwardIterator first, ForwardIterator last, RandomAccessIterator result, Map map, Array & counters)
        {
            dispose(std::make_move_iterator(first), std::make_move_iterator(last), result, map, counters);
        }

        template <typename ForwardIterator, typename RandomAccessIterator, typename Map, typename Dispose>
        RandomAccessIterator counting_sort_impl (ForwardIterator first, ForwardIterator last, RandomAccessIterator result, Map map, Dispose dispose)
        {
            using value_type = typename std::iterator_traits<ForwardIterator>::value_type;
            using traits = counting_sort_traits<value_type, Map>;

            using difference_type = typename std::iterator_traits<RandomAccessIterator>::difference_type;
            // Единица для дополнительного нуля в начале массива.
            difference_type counters[traits::value_range + 1] = {0};

            collect(first, last, map, counters);
            dispose(first, last, result, map, counters);

            return result + burst::cback(counters);
        }

        template <typename ForwardIterator, typename RandomAccessIterator, typename Map>
        RandomAccessIterator counting_sort_copy_impl (ForwardIterator first, ForwardIterator last, RandomAccessIterator result, Map map)
        {
            return counting_sort_impl(first, last, result, map,
                [] (auto && ... xs)
                {
                    return dispose(std::forward<decltype(xs)>(xs)...);
                });
        }

        template <typename ForwardIterator, typename RandomAccessIterator, typename Map>
        RandomAccessIterator counting_sort_move_impl (ForwardIterator first, ForwardIterator last, RandomAccessIterator result, Map map)
        {
            return counting_sort_impl(first, last, result, map,
                [] (auto && ... xs)
                {
                    return dispose_move(std::forward<decltype(xs)>(xs)...);
                });
        }
    } // namespace detail
} // namespace burst

#endif // BURST_ALGORITHM_SORTING_DETAIL_COUNTING_SORT_HPP
