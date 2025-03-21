// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "extension.hpp"
#include "utils.hpp"

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "openvino/frontend/extension/conversion.hpp"
#include "openvino/frontend/tensorflow/extension/conversion.hpp"
#include "openvino/frontend/tensorflow/extension/op.hpp"

namespace py = pybind11;

using namespace ov::frontend::tensorflow;


void regclass_frontend_tensorflow_ConversionExtension(py::module m) {
    py::class_<ConversionExtension, ConversionExtension::Ptr, ov::frontend::ConversionExtensionBase> _ext(
        m,
        "_ConversionExtensionTensorflow",
        py::dynamic_attr());
    class PyConversionExtension : public ConversionExtension {
    public:
        using Ptr = std::shared_ptr<PyConversionExtension>;
        using PyCreatorFunction = std::function<ov::OutputVector(const ov::frontend::NodeContext*)>;
        PyConversionExtension(const std::string& op_type, const PyCreatorFunction& f)
            : ConversionExtension(op_type, [f](const ov::frontend::NodeContext& node) -> ov::OutputVector {
                  return f(static_cast<const ov::frontend::NodeContext*>(&node));
              }) {}
    };
    py::class_<PyConversionExtension, PyConversionExtension::Ptr, ConversionExtension> ext(
        m,
        "ConversionExtensionTensorflow",
        py::dynamic_attr());

    ext.def(py::init([](const std::string& op_type, const PyConversionExtension::PyCreatorFunction& f) {
        return std::make_shared<PyConversionExtension>(op_type, f);
    }));
}

void regclass_frontend_tensorflow_OpExtension(py::module m) {
    py::class_<OpExtension<void>, std::shared_ptr<OpExtension<void>>, ConversionExtension> ext(
            m,
            "OpExtensionTensorflow",
            py::dynamic_attr());

    ext.def(py::init([](const std::string& fw_type_name,
                        const std::map<std::string, std::string>& attr_names_map,
                        const std::map<std::string, py::object>& attr_values_map) {
                std::map<std::string, ov::Any> any_map;
                for (const auto& it : attr_values_map) {
                    any_map[it.first] = Common::utils::py_object_to_any(it.second);
                }
                return std::make_shared<OpExtension<void>>(fw_type_name, attr_names_map, any_map);
            }), py::arg("fw_type_name"),
            py::arg("attr_names_map") = std::map<std::string, std::string>(),
            py::arg("attr_values_map") = std::map<std::string, ov::Any>());

    ext.def(py::init([](const std::string& ov_type_name,
                        const std::string& fw_type_name,
                        const std::map<std::string, std::string>& attr_names_map,
                        const std::map<std::string, py::object>& attr_values_map) {
                std::map<std::string, ov::Any> any_map;
                for (const auto& it : attr_values_map) {
                    any_map[it.first] = Common::utils::py_object_to_any(it.second);
                }
                return std::make_shared<OpExtension<void>>(ov_type_name, fw_type_name, attr_names_map, any_map);
            }),
            py::arg("ov_type_name"),
            py::arg("fw_type_name"),
            py::arg("attr_names_map") = std::map<std::string, std::string>(),
            py::arg("attr_values_map") = std::map<std::string, py::object>());
}
