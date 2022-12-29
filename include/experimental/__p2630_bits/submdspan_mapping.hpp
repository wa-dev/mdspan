
namespace std {
namespace experimental {

//******************************************
// Return type of submdspan_mapping overloads
//******************************************
template <class Mapping> struct mapping_offset {
  Mapping map;
  size_t offset;
};

namespace detail {
// constructs sub strides
template <class SrcMapping, size_t... InvMapIdxs>
constexpr auto construct_sub_strides(const SrcMapping &src_map,
                                     index_sequence<InvMapIdxs...>) {
  return array<typename SrcMapping::index_type, sizeof...(InvMapIdxs)>{
      src_map.stride(InvMapIdxs)...};
}
} // namespace detail

//**********************************
// layout_left submdspan_mapping
//*********************************
namespace detail {

// Figure out whether to preserve layout_left
template <class IndexSequence, size_t SubRank, class... SliceSpecifiers>
struct preserve_layout_left_mapping;

template <class... SliceSpecifiers, size_t... Idx, size_t SubRank>
struct preserve_layout_left_mapping<index_sequence<Idx...>, SubRank,
                                    SliceSpecifiers...> {
  constexpr static bool value =
      (SubRank == 0) ||
      ((Idx < SubRank - 1
            ? is_same_v<SliceSpecifiers, full_extent_t>
            : (Idx == SubRank - 1 ? is_same_v<SliceSpecifiers, full_extent_t> ||
                                        is_convertible_v<SliceSpecifiers,
                                                         tuple<size_t, size_t>>
                                  : true)) &&
       ...);
};
} // namespace detail

// Actual submdspan mapping call
template <class Extents, class... SliceSpecifiers>
constexpr auto
submdspan_mapping(const layout_left::mapping<Extents> &src_mapping,
                  SliceSpecifiers... slices) {

  // compute sub extents
  using src_ext_t = Extents;
  auto dst_ext = submdspan_extents(src_mapping.extents(), slices...);
  using dst_ext_t = decltype(dst_ext);

  // figure out sub layout type
  constexpr bool preserve_layout = detail::preserve_layout_left_mapping<
      decltype(make_index_sequence<src_ext_t::rank()>()), dst_ext_t::rank(),
      SliceSpecifiers...>::value;
  using dst_layout_t =
      conditional_t<preserve_layout, layout_left, layout_stride>;

  if constexpr (is_same_v<dst_layout_t, layout_left>) {
    // return layout_left again
    return mapping_offset{
        typename dst_layout_t::template mapping<dst_ext_t>(dst_ext),
        static_cast<size_t>(src_mapping(detail::first_of(slices)...))};
  } else {
    // return layout stride: need inv_mapping to get strides
    using inv_map_t = typename detail::inv_map_rank<0, index_sequence<>,
                                                    SliceSpecifiers...>::type;
    return mapping_offset{
        typename dst_layout_t::template mapping<dst_ext_t>(
            dst_ext, detail::construct_sub_strides(src_mapping, inv_map_t())),
        static_cast<size_t>(src_mapping(detail::first_of(slices)...))};
  }
}

//**********************************
// layout_right submdspan_mapping
//*********************************
namespace detail {

// Figure out whether to preserve layout_left
template <class IndexSequence, size_t SubRank, class... SliceSpecifiers>
struct preserve_layout_right_mapping;

template <class... SliceSpecifiers, size_t... Idx, size_t SubRank>
struct preserve_layout_right_mapping<index_sequence<Idx...>, SubRank,
                                     SliceSpecifiers...> {
  constexpr static size_t SrcRank = sizeof...(SliceSpecifiers);
  constexpr static bool value =
      (SubRank == 0) ||
      ((Idx > SrcRank - SubRank
            ? is_same_v<SliceSpecifiers, full_extent_t>
            : (Idx == SrcRank - SubRank
                   ? is_same_v<SliceSpecifiers, full_extent_t> ||
                         is_convertible_v<SliceSpecifiers,
                                          tuple<size_t, size_t>>
                   : true)) &&
       ...);
};
} // namespace detail

template <class Extents, class... SliceSpecifiers>
constexpr auto
submdspan_mapping(const layout_right::mapping<Extents> &src_mapping,
                  SliceSpecifiers... slices) {

  // get sub extents
  using src_ext_t = Extents;
  auto dst_ext = submdspan_extents(src_mapping.extents(), slices...);
  using dst_ext_t = decltype(dst_ext);

  // determine new layout type
  constexpr bool preserve_layout = detail::preserve_layout_right_mapping<
      decltype(make_index_sequence<src_ext_t::rank()>()), dst_ext_t::rank(),
      SliceSpecifiers...>::value;
  using dst_layout_t =
      conditional_t<preserve_layout, layout_right, layout_stride>;

  if constexpr (is_same_v<dst_layout_t, layout_right>) {
    // layout_right case
    return mapping_offset{
        typename dst_layout_t::template mapping<dst_ext_t>(dst_ext),
        static_cast<size_t>(src_mapping(detail::first_of(slices)...))};
  } else {
    // layout_stride case
    using inv_map_t = typename detail::inv_map_rank<0, index_sequence<>,
                                                    SliceSpecifiers...>::type;
    return mapping_offset{
        typename dst_layout_t::template mapping<dst_ext_t>(
            dst_ext, detail::construct_sub_strides(src_mapping, inv_map_t())),
        static_cast<size_t>(src_mapping(detail::first_of(slices)...))};
  }
}

//**********************************
// layout_stride submdspan_mapping
//*********************************
template <class Extents, class... SliceSpecifiers>
constexpr auto
submdspan_mapping(const layout_stride::mapping<Extents> &src_mapping,
                  SliceSpecifiers... slices) {
  auto dst_ext = submdspan_extents(src_mapping.extents(), slices...);
  using dst_ext_t = decltype(dst_ext);
  using inv_map_t = typename detail::inv_map_rank<0, index_sequence<>,
                                                  SliceSpecifiers...>::type;
  return mapping_offset{
      layout_stride::mapping<dst_ext_t>(
          dst_ext, detail::construct_sub_strides(src_mapping, inv_map_t())),
      static_cast<size_t>(src_mapping(detail::first_of(slices)...))};
}
} // namespace experimental
} // namespace std
