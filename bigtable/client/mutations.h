// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef BIGTABLE_CLIENT_MUTATIONS_H_
#define BIGTABLE_CLIENT_MUTATIONS_H_

#include <bigtable/client/version.h>

#include <google/bigtable/v2/bigtable.pb.h>
#include <google/bigtable/v2/data.pb.h>
#include <grpc++/grpc++.h>

#include <type_traits>

namespace bigtable {
inline namespace BIGTABLE_CLIENT_NS {

/**
 * Represent a single change to a specific row in a Table.
 *
 * Mutations come in different forms, they can set a specific cell,
 * delete a specific cell or delete multiple cells in a row.
 */
struct Mutation {
  google::bigtable::v2::Mutation op;
};

/// Create a mutation to set a cell value.
Mutation SetCell(std::string family, std::string column, std::int64_t timestamp,
                 std::string value);

//@{
/**
 * @name Create mutations to delete a range of cells from a column.
 *
 * The following functions create a mutation that deletes all the
 * cells in the given column family and column within the given
 * timestamp range.
 *
 * The ending timestamp is exclusive, while the beginning timestamp is
 * inclusive.  Please notice that [a,a) is an invalid range unless
 * a==0.
 */
/// Delete only within the timestamp range provided.
Mutation DeleteFromColumn(std::string family, std::string column,
                          std::int64_t timestamp_begin,
                          std::int64_t timestamp_end);
/// Delete all the values for the column.
Mutation DeleteFromColumn(std::string family, std::string column);
/// Delete starting from, and including, @a timestamp_begin.
Mutation DeleteFromColumnStartingFrom(std::string family, std::string column,
                                      std::int64_t timestamp_begin);
/// Delete up to, but excluding, @a timestamp_end.
Mutation DeleteFromColumnEndingAt(std::string family, std::string column,
                                  std::int64_t timestamp_end);
//@}

/// Create a mutation to delete all the cells in a column family.
Mutation DeleteFromFamily(std::string family);

/// Create a mutation to delete all the cells in a row.
Mutation DeleteFromRow();

/**
 * Represent a single row mutation.
 *
 * Bigtable can perform multiple changes to a single row atomically.
 * This class represents 0 or more changes to apply to a single row.
 * The changes may include setting cells (which implicitly insert the
 * values), deleting values, etc.
 */
class SingleRowMutation {
 public:
  /// Create an empty mutation.
  explicit SingleRowMutation(std::string row_key)
      : row_key_(std::move(row_key)) {}

  /// Create a row mutation from a range of Mutations.
  template <typename iterator>
  SingleRowMutation(std::string row_key, iterator begin, iterator end)
      : row_key_(std::move(row_key)), ops_(begin, end) {}

  /// Create a row mutation from a initializer list.
  SingleRowMutation(std::string row_key, std::initializer_list<Mutation> list)
      : row_key_(std::move(row_key)) {
    for (auto&& i : list) {
      *ops_.Add() = i.op;
    }
  }

  /// Create a row mutation from gRPC proto
  SingleRowMutation(::google::bigtable::v2::MutateRowsRequest::Entry&& entry)
      : row_key_(std::move(*entry.mutable_row_key())), ops_() {
    ops_.Swap(entry.mutable_mutations());
  }

  // Add a mutation at the end.
  SingleRowMutation& emplace_back(Mutation&& mut) {
    ops_.Add()->Swap(&mut.op);
    return *this;
  }

  // Get the row key.
  std::string const& row_key() const { return row_key_; }

  friend class Table;

  SingleRowMutation(SingleRowMutation&& rhs) = default;
  SingleRowMutation& operator=(SingleRowMutation&& rhs) = default;
  SingleRowMutation(SingleRowMutation const& rhs) = default;
  SingleRowMutation& operator=(SingleRowMutation const& rhs) = default;

  /// Move the contents into a bigtable::v2::MutateRowsRequest::Entry.
  void MoveTo(google::bigtable::v2::MutateRowsRequest::Entry* entry) {
    entry->set_row_key(std::move(row_key_));
    entry->mutable_mutations()->Swap(&ops_);
  }

 private:
  std::string row_key_;
  google::protobuf::RepeatedPtrField<google::bigtable::v2::Mutation> ops_;
};

}  // namespace BIGTABLE_CLIENT_NS
}  // namespace bigtable

#endif  // BIGTABLE_CLIENT_MUTATIONS_H_