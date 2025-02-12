// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "core/operator_set.hpp"
#include "openvino/op/convert.hpp"
#include "openvino/op/divide.hpp"
#include "openvino/op/gather.hpp"
#include "openvino/op/log.hpp"
#include "openvino/op/multiply.hpp"
#include "openvino/op/negative.hpp"
#include "openvino/op/not_equal.hpp"
#include "openvino/op/reduce_mean.hpp"
#include "openvino/op/reduce_sum.hpp"
#include "openvino/op/softmax.hpp"
#include "softmax_cross_entropy_loss.hpp"

namespace ov {
namespace frontend {
namespace onnx {
namespace {
OutputVector impl_softmax_cross_entropy(const Node& node, int64_t axis_default) {
    const auto inputs = node.get_ov_inputs();

    const auto scores = inputs[0];
    const auto labels = inputs[1];

    bool has_weights = inputs.size() > 2;
    std::shared_ptr<ov::Node> weights_gather = nullptr;

    bool has_ignore_index = node.has_attribute("ignore_index");
    int64_t ignore_index_val = 0;
    std::shared_ptr<ov::Node> mask = nullptr;

    if (has_ignore_index) {
        ignore_index_val = node.get_attribute_value<int64_t>("ignore_index");
        auto ignore_index_node = ov::op::v0::Constant::create(labels.get_element_type(), {}, {ignore_index_val});
        auto neq = std::make_shared<ov::op::v1::NotEqual>(labels, ignore_index_node);
        mask = std::make_shared<ov::op::v0::Convert>(neq, scores.get_element_type());
    }

    if (has_weights) {
        const auto weights = inputs[2];
        const auto axis_for_weights = ov::op::v0::Constant::create(element::i64, {}, {0});
        weights_gather = std::make_shared<ov::op::v8::Gather>(weights, labels, axis_for_weights);

        if (has_ignore_index) {
            weights_gather = std::make_shared<ov::op::v1::Multiply>(weights_gather, mask);
        }
    } else if (has_ignore_index) {
        weights_gather = mask;
    }

    const auto axis = node.get_attribute_value<int64_t>("axis", axis_default);
    const auto reduction = node.get_attribute_value<std::string>("reduction", "mean");

    const auto softmax = std::make_shared<ov::op::v8::Softmax>(scores, axis);
    const auto log_softmax = std::make_shared<ov::op::v0::Log>(softmax);

    const auto axis_const = ov::op::v0::Constant::create(element::i64, {}, {axis});
    const auto gathered = std::make_shared<ov::op::v8::Gather>(log_softmax, labels, axis_const);

    std::shared_ptr<ov::Node> loss = std::make_shared<ov::op::v0::Negative>(gathered);

    if (weights_gather) {
        loss = std::make_shared<ov::op::v1::Multiply>(loss, weights_gather);
    }

    if (reduction != "none") {
        auto loss_shape = loss->get_output_partial_shape(0);
        if (loss_shape.rank().is_static()) {
            size_t loss_rank = loss_shape.rank().get_length();
            std::vector<int64_t> reduce_axes(loss_rank);
            std::iota(reduce_axes.begin(), reduce_axes.end(), 0);
            auto reduce_axis = ov::op::v0::Constant::create(ov::element::i64, {reduce_axes.size()}, reduce_axes);

            if (reduction == "mean") {
                if (weights_gather) {
                    auto loss_sum = std::make_shared<ov::op::v1::ReduceSum>(loss, reduce_axis, false);
                    auto weight_sum = std::make_shared<ov::op::v1::ReduceSum>(weights_gather, reduce_axis, false);
                    loss = std::make_shared<ov::op::v1::Divide>(loss_sum, weight_sum);
                } else {
                    loss = std::make_shared<ov::op::v1::ReduceMean>(loss, reduce_axis, false);
                }
            } else if (reduction == "sum") {
                loss = std::make_shared<ov::op::v1::ReduceSum>(loss, reduce_axis, false);
            }
        } else {
            OPENVINO_THROW("Dynamic rank is not supported for SoftmaxCrossEntropyLoss reduction.");
        }
    }

    return {loss};
}
}  // namespace
namespace ai_onnx {
namespace opset_12 {
OutputVector softmax_cross_entropy_loss(const Node& node) {
    return impl_softmax_cross_entropy(node, 1);
}
ONNX_OP("SoftmaxCrossEntropyLoss", OPSET_IN(12), ai_onnx::opset_12::softmax_cross_entropy_loss);
}  // namespace opset_12
namespace opset_13 {
OutputVector softmax_cross_entropy_loss(const Node& node) {
    return impl_softmax_cross_entropy(node, 1);
}
ONNX_OP("SoftmaxCrossEntropyLoss", OPSET_IN(13), ai_onnx::opset_13::softmax_cross_entropy_loss);
}  // namespace opset_13
}  // namespace ai_onnx
}  // namespace onnx
}  // namespace frontend
}  // namespace ov