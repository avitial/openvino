//*****************************************************************************
// Copyright 2017-2020 Intel Corporation
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
//*****************************************************************************

#include "ngraph/op/avg_pool.hpp"
#include "ngraph/attribute_visitor.hpp"
#include "ngraph/graph_util.hpp"
#include "ngraph/validation_util.hpp"

using namespace std;
using namespace ngraph;

// *** AvgPool OP SET 1 ***
NGRAPH_RTTI_DEFINITION(op::v1::AvgPool, "AvgPool", 1);

op::v1::AvgPool::AvgPool(const Output<Node>& arg,
                         const Strides& strides,
                         const Shape& pads_begin,
                         const Shape& pads_end,
                         const Shape& kernel,
                         bool exclude_pad,
                         op::RoundingType rounding_type,
                         const PadType& auto_pad)
    : Op({arg})
    , m_kernel(kernel)
    , m_strides(strides)
    , m_pads_begin(pads_begin)
    , m_pads_end(pads_end)
    , m_exclude_pad(exclude_pad)
    , m_auto_pad(auto_pad)
    , m_rounding_type(rounding_type)
{
    constructor_validate_and_infer_types();
}

op::v1::AvgPool::AvgPool(const Output<Node>& arg,
                         const Strides& strides,
                         const Shape& pads_begin,
                         const Shape& pads_end,
                         const Shape& kernel,
                         bool exclude_pad,
                         op::RoundingType rounding_type)
    : AvgPool(arg,
              strides,
              pads_begin,
              pads_end,
              kernel,
              exclude_pad,
              rounding_type,
              op::PadType::EXPLICIT)
{
}

bool op::v1::AvgPool::visit_attributes(AttributeVisitor& visitor)
{
    visitor.on_attribute("kernel", m_kernel);
    visitor.on_attribute("strides", m_strides);
    visitor.on_attribute("pads_begin", m_pads_begin);
    visitor.on_attribute("pads_end", m_pads_end);
    visitor.on_attribute("exclude_pad", m_exclude_pad);
    visitor.on_attribute("auto_pad", m_auto_pad);
    visitor.on_attribute("rounding_type", m_rounding_type);
    return true;
}

void op::v1::AvgPool::validate_and_infer_types()
{
    if (0 == m_strides.size())
    {
        m_strides = Strides(m_kernel.size(), 1);
    }

    if (0 == m_pads_begin.size())
    {
        m_pads_begin = Shape(m_kernel.size(), 0);
    }

    if (0 == m_pads_end.size())
    {
        m_pads_end = Shape(m_kernel.size(), 0);
    }

    const PartialShape& arg_shape = get_input_partial_shape(0);

    if (m_auto_pad == PadType::SAME_UPPER || m_auto_pad == PadType::SAME_LOWER)
    {
        if (arg_shape.is_static())
        {
            CoordinateDiff pads_end, pads_begin;
            infer_auto_padding(arg_shape.to_shape(),
                               m_kernel,
                               m_strides,
                               Strides(m_kernel.size(), 1), // No dilation
                               m_auto_pad,
                               pads_end,
                               pads_begin);
            m_pads_end = Shape(pads_end.begin(), pads_end.end());
            m_pads_begin = Shape(pads_begin.begin(), pads_begin.end());
        }
    }

    // infer_batched_forward_pooling wants CoordinateDiffs for these, while the pooling ops for
    // now still take Shape (no negative padding).
    CoordinateDiff pads_begin(m_pads_begin.begin(), m_pads_begin.end());
    CoordinateDiff pads_end(m_pads_end.begin(), m_pads_end.end());

    set_output_type(0,
                    get_input_element_type(0),
                    infer_batched_pooling_forward(this,
                                                  arg_shape,
                                                  pads_begin,
                                                  pads_end,
                                                  m_kernel,
                                                  m_strides,
                                                  !m_exclude_pad,
                                                  m_rounding_type == op::RoundingType::CEIL));
}

const Shape& op::v1::AvgPool::get_kernel() const
{
    return m_kernel;
}

void op::v1::AvgPool::set_kernel(const Shape& kernel)
{
    m_kernel = kernel;
}

const Strides& op::v1::AvgPool::get_strides() const
{
    return m_strides;
}

void op::v1::AvgPool::set_strides(const Strides& strides)
{
    m_strides = strides;
}

const Shape& op::v1::AvgPool::get_pads_begin() const
{
    return m_pads_begin;
}

void op::v1::AvgPool::set_pads_begin(const Shape& pads_begin)
{
    m_pads_begin = pads_begin;
}

const Shape& op::v1::AvgPool::get_pads_end() const
{
    return m_pads_end;
}

void op::v1::AvgPool::set_pads_end(const Shape& pads_end)
{
    m_pads_end = pads_end;
}

bool op::v1::AvgPool::get_exclude_pad() const
{
    return m_exclude_pad;
}

void op::v1::AvgPool::set_exclude_pad(bool exclude_pad)
{
    m_exclude_pad = exclude_pad;
}

const op::PadType& op::v1::AvgPool::get_auto_pad() const
{
    return m_auto_pad;
}

void op::v1::AvgPool::set_auto_pad(const op::PadType& auto_pad)
{
    m_auto_pad = auto_pad;
}

op::RoundingType op::v1::AvgPool::get_rounding_type() const
{
    return m_rounding_type;
}

void op::v1::AvgPool::set_rounding_type(op::RoundingType rounding_type)
{
    m_rounding_type = rounding_type;
}

shared_ptr<Node> op::v1::AvgPool::clone_with_new_inputs(const OutputVector& new_args) const
{
    check_new_args_count(this, new_args);
    return make_shared<v1::AvgPool>(new_args.at(0),
                                    m_strides,
                                    m_pads_begin,
                                    m_pads_end,
                                    m_kernel,
                                    m_exclude_pad,
                                    m_rounding_type,
                                    m_auto_pad);
}

shared_ptr<Node> op::v1::AvgPool::get_default_value() const
{
    return op::Constant::create(get_element_type(), get_shape(), {0});
}
