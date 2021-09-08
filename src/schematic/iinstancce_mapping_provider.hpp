#pragma once

namespace horizon {
class IInstanceMappingProvider {
public:
    virtual const class BlockInstanceMapping *get_block_instance_mapping() const = 0;
    virtual unsigned int get_sheet_index(const class UUID &sheet) const = 0;
    virtual unsigned int get_sheet_total() const = 0;
};
} // namespace horizon
