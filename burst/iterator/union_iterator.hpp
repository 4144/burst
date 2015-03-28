#ifndef BURST_ITERATOR_UNION_ITERATOR_HPP
#define BURST_ITERATOR_UNION_ITERATOR_HPP

#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/algorithm/cxx11/is_sorted.hpp>
#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/range/algorithm/upper_bound.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <burst/iterator/detail/front_value_compare.hpp>
#include <burst/iterator/end_tag.hpp>
#include <burst/range/skip_to_upper_bound.hpp>

namespace burst
{
    //!     Итератор объединения.
    /*!
            Предназначен для объединения нескольких диапазонов одного типа "на лету", то есть без
        использования дополнительной памяти для хранения результирующего диапазона.
            Принимает на вход набор упорядоченных диапазонов и перемещается по элементам этих
        диапазонов так, что каждый элемент исходных диапазонов встречается столько раз, сколько он
        встретился в диапазоне, в котором он встречается чаще других. Проще говоря, объединяемые
        диапазоны рассматриваются как мультимножества.
            Между элементами результирующего диапазона сохраняется заданное отношение порядка.
            Полученный в результате объединения диапазон неизменяем. То есть из итератора можно
        только прочитать значения, но нельзя записать. Такое ограничение обусловлено тем, что
        каждый выдаваемый итератором элемент может присутствовать в нескольких из входных
        диапазонов, а значит, нельзя однозначно выбрать из них какой-то один для записи нового
        значения.

        \tparam InputRange
            Тип принимаемого на вход диапазона. Он должен быть хотя бы однопроходным, то есть
            удовлетворять требованиям понятия "Single Pass Range".
        \tparam Compare
            Бинарная операция, задающая отношение строгого порядка на элементах диапазона
            InputRange.
            Если пользователем явно не указана операция, то, по-умолчанию, берётся отношение
            "меньше", задаваемое функциональным объектом "std::less<T>".

            Алгоритм работы.

        1. Диапазоны склыдываются в массив, в котором они упорядочены по первому элементу в
           заданном отношении порядка.
           В каждый момент времени первый элемент первого диапазона — текущий элемент объединения.
        2. Чтобы найти следующий элемент объединения, нужно продвинуть на один элемент вперёд все
           диапазоны, у которых первый элемент совпадает с текущим элементом объединения, а затем
           заново отсортировать массив диапазонов.
           Если в результате продвижения какой-либо из диапазонов опустел, он выбрасывается.
        3. Когда все элементы опустели, объединение закончено.
     */
    template
    <
        typename InputRange,
        typename Compare = std::less<typename InputRange::value_type>
    >
    class union_iterator:
        public boost::iterator_facade
        <
            union_iterator<InputRange, Compare>,
            typename InputRange::value_type,
            boost::forward_traversal_tag,
            typename std::conditional
            <
                std::is_lvalue_reference<typename InputRange::reference>::value,
                typename std::remove_reference<typename InputRange::reference>::type const &,
                typename InputRange::reference
            >
            ::type
        >
    {
    public:
        using range_type = InputRange;
        using compare_type = Compare;

        using base_type =
            boost::iterator_facade
            <
                union_iterator<range_type, Compare>,
                typename range_type::value_type,
                boost::forward_traversal_tag,
                typename std::conditional
                <
                    std::is_lvalue_reference<typename range_type::reference>::value,
                    typename std::remove_reference<typename range_type::reference>::type const &,
                    typename range_type::reference
                >
                ::type
            >;

    public:
        template <typename RangeOfRanges>
        union_iterator (RangeOfRanges ranges, compare_type compare = compare_type()):
            m_ranges(),
            m_compare(compare)
        {
            BOOST_STATIC_ASSERT(std::is_same<typename RangeOfRanges::value_type, range_type>::value);
            BOOST_ASSERT(boost::algorithm::all_of(ranges, boost::bind(&boost::algorithm::is_sorted<range_type, compare_type>, _1, m_compare)));

            auto is_not_empty = [] (const auto & range) { return not range.empty(); };
            auto non_empty_ranges =
                boost::make_iterator_range
                (
                    boost::make_filter_iterator(is_not_empty, ranges.begin(), ranges.end()),
                    boost::make_filter_iterator(is_not_empty, ranges.end(), ranges.end())
                );

            m_ranges.reserve(ranges.size());
            std::move(non_empty_ranges.begin(), non_empty_ranges.end(), std::back_inserter(m_ranges));
            std::sort(m_ranges.begin(), m_ranges.end(), detail::compare_by_front_value(m_compare));
        }

        union_iterator () = default;

    private:
        friend class boost::iterator_core_access;

        void increment ()
        {
            auto next_element = boost::upper_bound(m_ranges, m_ranges.front(), detail::compare_by_front_value(m_compare));

            auto value_to_skip_by = m_ranges.front().front();
            std::for_each(m_ranges.begin(), next_element,
                [this, & value_to_skip_by] (range_type & range)
                {
                    range.advance_begin(1);
                });

            auto is_empty = [] (const auto & range) { return range.empty(); };
            m_ranges.erase
            (
                std::remove_if(m_ranges.begin(), m_ranges.end(), is_empty),
                m_ranges.end()
            );

            std::sort(m_ranges.begin(), m_ranges.end(), detail::compare_by_front_value(m_compare));
        }

    private:
        typename base_type::reference dereference () const
        {
            return m_ranges.front().front();
        }

        bool equal (const union_iterator & that) const
        {
            return this->m_ranges == that.m_ranges;
        }

    private:
        std::vector<range_type> m_ranges;
        compare_type m_compare;

    };

    //!     Функция для создания итератора объединения с предикатом.
    /*!
            Принимает на вход набор диапазонов, которые нужно объединить, и операцию, задающую
        отношение строгого порядка на элементах этого диапазона.
            Сами диапазоны должны быть упорядочены относительно этой операции.
            Возвращает итератор на первый элемент объединения входных диапазонов.
     */
    template <typename RangeOfRanges, typename Compare>
    union_iterator
    <
        typename std::remove_reference<RangeOfRanges>::type::value_type,
        Compare
    >
    make_union_iterator (RangeOfRanges && ranges, Compare compare)
    {
        return union_iterator<typename std::remove_reference<RangeOfRanges>::type::value_type, Compare>
        (
            std::forward<RangeOfRanges>(ranges),
            compare
        );
    }

    //!     Функция для создания итератора на конец объединения с предикатом.
    /*!
            Принимает на вход набор диапазонов, предикат и индикатор конца итератора.
            Набор диапазонов и предикат не используются, они нужны только для автоматического
        вывода типа итератора.
            Возвращает итератор-конец, который, если до него дойти, покажет, что элементы
        объединения закончились.
     */
    template <typename RangeOfRanges, typename Compare>
    union_iterator
    <
        typename std::remove_reference<RangeOfRanges>::type::value_type,
        Compare
    >
    make_union_iterator (RangeOfRanges &&, Compare, iterator::end_tag_t)
    {
        return union_iterator<typename std::remove_reference<RangeOfRanges>::type::value_type, Compare>();
    }

    //!     Функция для создания итератора объединения.
    /*!
            Принимает на вход набор диапазонов, которые нужно объединить.
            Возвращает итератор на первый элемент объединения входных диапазонов.
            Отношение порядка для элементов диапазона выбирается по-умолчанию.
     */
    template <typename RangeOfRanges>
    union_iterator
    <
        typename std::remove_reference<RangeOfRanges>::type::value_type
    >
    make_union_iterator (RangeOfRanges && ranges)
    {
        return union_iterator<typename std::remove_reference<RangeOfRanges>::type::value_type>
        (
            std::forward<RangeOfRanges>(ranges)
        );
    }

    //!     Функция для создания итератора на конец объединения.
    /*!
            Принимает на вход набор диапазонов, который не используется, а нужен только для
        автоматического вывода типа итератора.
            Возвращает итератор на конец объединённой последовательности.
            Отношение порядка берётся по-умолчанию.
     */
    template <typename RangeOfRanges>
    union_iterator
    <
        typename std::remove_reference<RangeOfRanges>::type::value_type
    >
    make_union_iterator (RangeOfRanges &&, iterator::end_tag_t)
    {
        return union_iterator<typename std::remove_reference<RangeOfRanges>::type::value_type>();
    }
} // namespace burst

#endif // BURST_ITERATOR_UNION_ITERATOR_HPP