#ifndef F2E7192A_3374_4162_830B_824517E3C389
#define F2E7192A_3374_4162_830B_824517E3C389

namespace cjf
{
  template <class DeviceType, typename RegAddressType, RegAddressType Address, typename ValueType>
  struct register_base
  {
    using address_type = RegAddressType;
    using device_type = DeviceType;
    using value_type = ValueType;
    static constexpr address_type address = Address;
    explicit operator RegAddressType() const noexcept { return address; }
  };

  template <class DeviceType, typename RegAddressType, RegAddressType Address, typename ValueType>
  struct read_only_register : public register_base<DeviceType, RegAddressType, Address, ValueType>
  {
  };

  template <class DeviceType, typename RegAddressType, RegAddressType Address, typename ValueType>
  struct read_write_register : public register_base<DeviceType, RegAddressType, Address, ValueType>
  {
  };

  template <class DeviceType, typename RegAddressType, RegAddressType Address, typename ValueType>
  struct write_only_register : public register_base<DeviceType, RegAddressType, Address, ValueType>
  {
  };

  template <class DeviceType, typename RegAddressType>
  class has_registers
  {
  protected:
    template <RegAddressType Address, typename ValueType>
    using reg_ro = read_only_register<DeviceType, RegAddressType, Address, ValueType>;

    template <RegAddressType Address, typename ValueType>
    using reg_rw = read_write_register<DeviceType, RegAddressType, Address, ValueType>;

    template <RegAddressType Address, typename ValueType>
    using reg_wo = write_only_register<DeviceType, RegAddressType, Address, ValueType>;
  };

} // namespace cjf

#endif /* F2E7192A_3374_4162_830B_824517E3C389 */
