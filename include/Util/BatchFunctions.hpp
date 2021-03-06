/**
 * @author Federico Busato                                                  <br>
 *         Univerity of Verona, Dept. of Computer Science                   <br>
 *         federico.busato@univr.it
 * @date August, 2017
 * @version v2
 *
 * @copyright Copyright © 2017 Hornet. All rights reserved.
 *
 * @license{<blockquote>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * </blockquote>}
 *
 * @file
 */
#pragma once

#include "BasicTypes.hpp"       //vid_t
#include "GraphIO/GraphStd.hpp" //GraphStd

namespace hornets_nest {

namespace detail {
    enum class BatchGenEnum { WEIGHTED = 1, PRINT = 2, UNIQUE = 4 };
} // namespace detail

class BatchGenProperty : public xlib::PropertyClass<detail::BatchGenEnum,
                                                     BatchGenProperty> {
public:
    explicit BatchGenProperty() noexcept = default;
    explicit BatchGenProperty(const detail::BatchGenEnum& obj) noexcept;
};

namespace batch_gen_property {
    const BatchGenProperty WEIGHTED (detail::BatchGenEnum::WEIGHTED);
    const BatchGenProperty PRINT    (detail::BatchGenEnum::PRINT);
    const BatchGenProperty UNIQUE   (detail::BatchGenEnum::UNIQUE);
}

enum class BatchGenType { INSERT, REMOVE };

void generateBatch(const graph::GraphStd<>& graph, int& batch_size,
                   vid_t* batch_src, vid_t* batch_dest,
                   const BatchGenType& batch_type,
                   const BatchGenProperty& prop = BatchGenProperty());

} // namespace hornets_nest
