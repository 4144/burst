#ifndef BURST_RANGE_BITAP_HPP
#define BURST_RANGE_BITAP_HPP

#include <burst/iterator/bitap_iterator.hpp>
#include <burst/iterator/end_tag.hpp>

#include <boost/range/iterator_range.hpp>

namespace burst
{
    //!     Функция для создания диапазона вхождений образца в текст.
    /*!
            Принимает на вход набор образец — диапазон, который нужно запомнить для дальнейшего
        поиска в тексте, и текст — диапазон, в котором будет происходить поиск.
            Возвращает диапазон, каждый элемент которого — это, в свою очередь, тоже диапазон,
        который задаёт места начала и конца в тексте, где встретилась последовательность элементов,
        идентичная образцу.
     */
    template <typename Bitmask, typename ForwardRange1, typename ForwardRange2>
    boost::iterator_range
    <
        bitap_iterator
        <
            decltype(algorithm::make_bitap<Bitmask>(std::declval<ForwardRange1>())),
            ForwardRange2
        >
    >
    bitap (const ForwardRange1 & pattern, const ForwardRange2 & text)
    {
        const auto bitap = algorithm::make_bitap<Bitmask>(pattern);

        return boost::make_iterator_range
        (
            make_bitap_iterator(bitap, text),
            make_bitap_iterator(bitap, text, iterator::end_tag)
        );
    }

    //!     Функция с произвольным отображением для создания диапазона вхождений образца в текст.
    /*!
            Принимает явный аргумент шаблона "Map", который обозначает тип, в котором будет
        храниться отображение элементов образца в битовые маски.
     */
    template <typename Bitmask, typename Map, typename ForwardRange1, typename ForwardRange2>
    boost::iterator_range
    <
        bitap_iterator
        <
            decltype(algorithm::make_bitap<Bitmask, Map>(std::declval<ForwardRange1>())),
            ForwardRange2
        >
    >
    bitap (const ForwardRange1 & pattern, const ForwardRange2 & text)
    {
        const auto bitap = algorithm::make_bitap<Bitmask, Map>(pattern);

        return boost::make_iterator_range
        (
            make_bitap_iterator(bitap, text),
            make_bitap_iterator(bitap, text, iterator::end_tag)
        );
    }
}

#endif // BURST_RANGE_BITAP_HPP
