#ifndef WEGIHT_MD5_CHECK_CONFIG_H
#define WEGIHT_MD5_CHECK_CONFIG_H

// Chip vendor decided capability for each data type

namespace android {
namespace nn {
namespace vsi_driver {

// Defalut value of weight Md5 checking is false
static constexpr bool weight_md5_check = false;
}  // namespace vsi_driver
}  // namespace nn
}  // namespace android

#endif