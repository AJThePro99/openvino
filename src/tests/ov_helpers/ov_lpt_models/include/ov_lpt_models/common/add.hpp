// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vector>

#include "openvino/core/shape.hpp"
#include "openvino/core/type/element_type.hpp"

namespace ov {
namespace builder {
namespace subgraph {

class Add {
public:
    Add();
    Add(const float value);
    Add(const std::vector<float>& values);
    Add(const std::vector<float>& values, const ov::element::Type outPrecision);
    Add(const std::vector<float>& values, const ov::element::Type outPrecision, const ov::Shape& constantShape);
    bool empty() const noexcept;

    std::vector<float> values;
    ov::element::Type outPrecision;
    ov::Shape constantShape;
    bool constantShapeIsDefined;
private:
    bool isEmpty;
};

}  // namespace subgraph
}  // namespace builder
}  // namespace ov
