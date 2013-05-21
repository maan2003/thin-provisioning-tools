#include "persistent-data/checksum.h"
#include "persistent-data/errors.h"
#include "thin-provisioning/superblock.h"
#include "thin-provisioning/superblock_validator.h"

using namespace thin_provisioning;
using namespace superblock_detail;

//----------------------------------------------------------------

void
superblock_traits::unpack(superblock_disk const &disk, superblock &value)
{
	value.csum_ = to_cpu<uint32_t>(disk.csum_);
	value.flags_ = to_cpu<uint32_t>(disk.flags_);
	value.blocknr_ = to_cpu<uint64_t>(disk.blocknr_);

	::memcpy(value.uuid_, disk.uuid_, sizeof(value.uuid_));
	value.magic_ = to_cpu<uint64_t>(disk.magic_);
	value.version_ = to_cpu<uint32_t>(disk.version_);
	value.time_ = to_cpu<uint32_t>(disk.time_);

	value.trans_id_ = to_cpu<uint64_t>(disk.trans_id_);
	value.metadata_snap_ = to_cpu<uint64_t>(disk.metadata_snap_);

	::memcpy(value.data_space_map_root_,
		 disk.data_space_map_root_,
		 sizeof(value.data_space_map_root_));
	::memcpy(value.metadata_space_map_root_,
		 disk.metadata_space_map_root_,
		 sizeof(value.metadata_space_map_root_));

	value.data_mapping_root_ = to_cpu<uint64_t>(disk.data_mapping_root_);
	value.device_details_root_ = to_cpu<uint64_t>(disk.device_details_root_);
	value.data_block_size_ = to_cpu<uint32_t>(disk.data_block_size_);

	value.metadata_block_size_ = to_cpu<uint32_t>(disk.metadata_block_size_);
	value.metadata_nr_blocks_ = to_cpu<uint64_t>(disk.metadata_nr_blocks_);

	value.compat_flags_ = to_cpu<uint32_t>(disk.compat_flags_);
	value.incompat_flags_ = to_cpu<uint32_t>(disk.incompat_flags_);
}

void
superblock_traits::pack(superblock const &value, superblock_disk &disk)
{
	disk.csum_ = to_disk<le32>(value.csum_);
	disk.flags_ = to_disk<le32>(value.flags_);
	disk.blocknr_ = to_disk<le64>(value.blocknr_);

	::memcpy(disk.uuid_, value.uuid_, sizeof(disk.uuid_));
	disk.magic_ = to_disk<le64>(value.magic_);
	disk.version_ = to_disk<le32>(value.version_);
	disk.time_ = to_disk<le32>(value.time_);

	disk.trans_id_ = to_disk<le64>(value.trans_id_);
	disk.metadata_snap_ = to_disk<le64>(value.metadata_snap_);

	::memcpy(disk.data_space_map_root_,
		 value.data_space_map_root_,
		 sizeof(disk.data_space_map_root_));
	::memcpy(disk.metadata_space_map_root_,
		 value.metadata_space_map_root_,
		 sizeof(disk.metadata_space_map_root_));

	disk.data_mapping_root_ = to_disk<le64>(value.data_mapping_root_);
	disk.device_details_root_ = to_disk<le64>(value.device_details_root_);
	disk.data_block_size_ = to_disk<le32>(value.data_block_size_);

	disk.metadata_block_size_ = to_disk<le32>(value.metadata_block_size_);
	disk.metadata_nr_blocks_ = to_disk<le64>(value.metadata_nr_blocks_);

	disk.compat_flags_ = to_disk<le32>(value.compat_flags_);
	disk.incompat_flags_ = to_disk<le32>(value.incompat_flags_);
}

//----------------------------------------------------------------

namespace {
	using namespace persistent_data;
	using namespace superblock_detail;

        uint32_t const VERSION = 1;
        unsigned const SECTOR_TO_BLOCK_SHIFT = 3;
	uint32_t const SUPERBLOCK_CSUM_SEED = 160774;

	struct sb_validator : public block_manager<>::validator {
		virtual void check(buffer<> const &b, block_address location) const {
			superblock_disk const *sbd = reinterpret_cast<superblock_disk const *>(&b);
			crc32c sum(SUPERBLOCK_CSUM_SEED);
			sum.append(&sbd->flags_, MD_BLOCK_SIZE - sizeof(uint32_t));
			if (sum.get_sum() != to_cpu<uint32_t>(sbd->csum_))
				throw checksum_error("bad checksum in superblock");
		}

		virtual void prepare(buffer<> &b, block_address location) const {
			superblock_disk *sbd = reinterpret_cast<superblock_disk *>(&b);
			crc32c sum(SUPERBLOCK_CSUM_SEED);
			sum.append(&sbd->flags_, MD_BLOCK_SIZE - sizeof(uint32_t));
			sbd->csum_ = to_disk<base::le32>(sum.get_sum());
		}
	};
}

block_manager<>::validator::ptr
thin_provisioning::superblock_validator()
{
	return block_manager<>::validator::ptr(new sb_validator);
}

//----------------------------------------------------------------