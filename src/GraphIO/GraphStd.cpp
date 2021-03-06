/**
 * @author Federico Busato                                                  <br>
 *         Univerity of Verona, Dept. of Computer Science                   <br>
 *         federico.busato@univr.it
 * @date June, 2017
 * @version v1.3
 *
 * @copyright Copyright © 2017 cuStinger. All rights reserved.
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
 */
#include "GraphIO/GraphStd.hpp"
#include "Host/Basic.hpp"      //ERROR
#include "Host/FileUtil.hpp"   //xlib::MemoryMapped
#include "Host/Numeric.hpp"    //xlib::per_cent
#include "Host/PrintExt.hpp"   //xlib::printArray
#include "Host/Statistics.hpp" //xlib::average
#include <algorithm>           //std::iota, std::shuffle
#include <cassert>             //assert
#include <chrono>              //std::chrono
#include <random>              //std::mt19937_64

namespace graph {

template<typename vid_t, typename eoff_t>
GraphStd<vid_t, eoff_t>::GraphStd(const eoff_t* csr_offsets, vid_t nV,
                                  const vid_t* csr_edges, eoff_t nE) noexcept :
                  GraphBase<vid_t, eoff_t>(nV, nE, structure_prop::UNDIRECTED) {
    allocate( { static_cast<size_t>(nV), static_cast<size_t>(nE),
                static_cast<size_t>(nE), structure_prop::UNDIRECTED } );
    std::copy(csr_offsets, csr_offsets + nV + 1, _out_offsets);
    std::copy(csr_edges, csr_edges + nE, _out_edges);
    for (vid_t i = 0; i < nV; i++)
        _out_degrees[i] = csr_offsets[i + 1] - csr_offsets[i];
}

template<typename vid_t, typename eoff_t>
GraphStd<vid_t, eoff_t>::GraphStd(StructureProp structure) noexcept :
                       GraphBase<vid_t, eoff_t>(std::move(structure)) {}

template<typename vid_t, typename eoff_t>
GraphStd<vid_t, eoff_t>::GraphStd(const char* filename,
                                  const ParsingProp& property)
                                  noexcept : GraphBase<vid_t, eoff_t>() {
    GraphBase<vid_t, eoff_t>::read(filename, property);
}

template<typename vid_t, typename eoff_t>
GraphStd<vid_t, eoff_t>::GraphStd(StructureProp structure,
                                  const char* filename,
                                  const ParsingProp& property)
                                  noexcept :
                                      GraphBase<vid_t, eoff_t>(structure) {
    GraphBase<vid_t, eoff_t>::read(filename, property);
}

//------------------------------------------------------------------------------

template<typename vid_t, typename eoff_t>
void GraphStd<vid_t, eoff_t>::allocate(const GInfo& ginfo) noexcept {
    assert(ginfo.num_vertices > 0 && ginfo.num_edges > 0);
    if (!_structure.is_direction_set())
        _structure += ginfo.direction;
    _undirected_to_directed = ginfo.direction == structure_prop::UNDIRECTED &&
                              _structure.is_directed();
    _directed_to_undirected = ginfo.direction == structure_prop::DIRECTED &&
                              _structure.is_undirected();
    size_t new_num_edges = ginfo.num_edges;
    if (_directed_to_undirected)
        new_num_edges = ginfo.num_edges * 2;
    else if (_undirected_to_directed) {
        _bitmask.init(ginfo.num_edges);
        _bitmask.randomize(_seed);
        new_num_edges = _bitmask.size();
    }

    xlib::check_overflow<vid_t>(ginfo.num_vertices);
    xlib::check_overflow<eoff_t>(new_num_edges);
    _nV = static_cast<vid_t>(ginfo.num_vertices);
    _nE = static_cast<eoff_t>(new_num_edges);

    if (_prop.is_print()) {
        const char* const dir[] = { "Structure: Undirected   ",
                                    "Structure: Directed     " };
        const char* graph_dir = ginfo.direction == structure_prop::UNDIRECTED
                                    ? dir[0] : dir[1];
        auto avg = static_cast<double>(ginfo.num_edges) / _nV;
        std::cout << "\n@File    V: " << std::left << std::setw(14)
                  << xlib::format(_nV)  << "E: " << std::setw(14)
                  << xlib::format(ginfo.num_edges) << graph_dir
                  << "avg. deg: " << xlib::format(avg, 1);
        if (_directed_to_undirected || _undirected_to_directed) {
            graph_dir =  _structure.is_undirected() ? dir[0] : dir[1];
            avg = static_cast<double>(new_num_edges) / _nV;
            std::cout << "\n@User    V: "  << std::left << std::setw(14)
                      << xlib::format(_nV) << "E: " << std::setw(14)
                      << xlib::format(new_num_edges) << graph_dir
                      << "avg. deg: " << xlib::format(avg) << "\n";
        }
        else
            assert(new_num_edges == ginfo.num_edges);
        std::cout << std::right << std::endl;
    }

    try {
        _out_offsets = new eoff_t[ _nV + 1 ];
        _out_edges   = new vid_t[ _nE ];
        _out_degrees = new degree_t[ _nV ]();
        _coo_edges   = new coo_t[ _nE ];
        if (_structure.is_undirected()) {
            _in_degrees = _out_degrees;
            _in_offsets = _out_offsets;
            _in_edges   = _out_edges;
        }
        else if (_structure.is_reverse()) {
            _in_offsets = new eoff_t[ _nV + 1 ];
            _in_edges   = new vid_t[ _nE ];
            _in_degrees = new degree_t[ _nV ]();
        }
    }
    catch (const std::bad_alloc&) {
        ERROR("OUT OF MEMORY: Graph too Large !!  V: ", _nV, " E: ", _nE)
    }
}

template<typename vid_t, typename eoff_t>
GraphStd<vid_t, eoff_t>::~GraphStd() noexcept {
    delete[] _out_offsets;
    delete[] _out_edges;
    delete[] _out_degrees;
    delete[] _coo_edges;
    if (_structure.is_directed() && _structure.is_reverse()) {
        delete[] _in_offsets;
        delete[] _in_edges;
        delete[] _in_degrees;
    }
}

template<typename vid_t, typename eoff_t>
void GraphStd<vid_t, eoff_t>::COOtoCSR() noexcept {
    if (_directed_to_undirected || _stored_undirected) {
        eoff_t half = _nE / 2;
        auto      k = half;
        for (eoff_t i = 0; i < half; i++) {
            auto src = _coo_edges[i].first;
            auto dst = _coo_edges[i].second;
            if (src == dst)
                continue;
            _coo_edges[k++] = {dst, src};
        }
        if (_prop.is_print() && _nE != k) {
            std::cout << "Double self-loops removed.  E: " << xlib::format(k)
                      << "\n";
        }
        _nE = k;

    }

    if (_directed_to_undirected) {
        if (_prop.is_print()) {
            if (_directed_to_undirected)
                std::cout << "Directed to Undirected: ";
            std::cout << "Removing duplicated edges..." << std::flush;
        }
        std::sort(_coo_edges, _coo_edges + _nE);
        auto   last = std::unique(_coo_edges, _coo_edges + _nE);
        auto new_nE = std::distance(_coo_edges, last);
        if (_prop.is_print() && new_nE != _nE) {
            std::cout << "(" << xlib::format(_nE - new_nE) << " edges removed)"
                      << std::endl;
        }
        _nE = new_nE;
    }
    else if (_undirected_to_directed) {
        std::cout << "Undirected to Directed: Removing random edges..."
                  << std::endl;
        for (eoff_t i = 0, k = 0; i < _nE; i++) {
            if (_bitmask[i])
                _coo_edges[k++] = _coo_edges[i];
        }
        _bitmask.free();
    }

    if (_prop.is_randomize()) {
        if (_prop.is_print())
            std::cout << "Randomization..." << std::endl;
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch()
                    .count();
        auto random_array = new vid_t[_nV];
        std::iota(random_array, random_array + _nV, 0);
        std::shuffle(random_array, random_array + _nV, std::mt19937_64(seed));
        for (eoff_t i = 0; i < _nE; i++) {
            _coo_edges[i].first  = random_array[ _coo_edges[i].first ];
            _coo_edges[i].second = random_array[ _coo_edges[i].second ];
        }
        delete[] random_array;
    }
    if (_prop.is_sort() && (!_directed_to_undirected || _prop.is_randomize())) {
        if (_prop.is_print())
            std::cout << "Sorting..." << std::endl;
        std::sort(_coo_edges, _coo_edges + _nE);
    }
    //--------------------------------------------------------------------------
    if (_prop.is_print())
        std::cout << "COO to CSR...\t" << std::flush;

    if (_structure.is_reverse() && _structure.is_directed()) {
        for (eoff_t i = 0; i < _nE; i++) {
            _out_degrees[_coo_edges[i].first]++;
            _in_degrees[ _coo_edges[i].second]++;
        }
    }
    else {
        for (eoff_t i = 0; i < _nE; i++)
            _out_degrees[_coo_edges[i].first]++;
    }

    _out_offsets[0] = 0;
    std::partial_sum(_out_degrees, _out_degrees + _nV, _out_offsets + 1);

    auto tmp = new degree_t[_nV]();
    for (eoff_t i = 0; i < _nE; i++) {
        vid_t  src = _coo_edges[i].first;
        vid_t dest = _coo_edges[i].second;
        _out_edges[ _out_offsets[src] + tmp[src]++ ] = dest;
    }

    if (_structure.is_directed() && _structure.is_reverse()) {
        _in_offsets[0] = 0;
        std::partial_sum(_in_degrees, _in_degrees + _nV, _in_offsets + 1);
        std::fill(tmp, tmp + _nV, 0);
        for (eoff_t i = 0; i < _nE; i++) {
            vid_t dest = _coo_edges[i].second;
            _in_edges[ _in_offsets[dest] + tmp[dest]++ ] = _coo_edges[i].first;
        }
    }
    delete[] tmp;
    if (!_structure.is_coo()) {
        delete[] _coo_edges;
        _coo_edges = nullptr;
    }
    if (_prop.is_print())
        std::cout << "Complete!\n" << std::endl;
}

template<typename vid_t, typename eoff_t>
void GraphStd<vid_t, eoff_t>::print() const noexcept {
    for (vid_t i = 0; i < _nV; i++) {
        std::cout << "[ " << i << " ] : ";
        for (eoff_t j = _out_offsets[i]; j < _out_offsets[i + 1]; j++)
            std::cout << _out_edges[j] << " ";
        std::cout << "\n";
    }
    std::cout << std::endl;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"

template<typename vid_t, typename eoff_t>
void GraphStd<vid_t, eoff_t>::print_raw() const noexcept {
    xlib::printArray(_out_offsets, _nV + 1, "Out-Offsets  ");           //NOLINT
    xlib::printArray(_out_edges,   _nE,     "Out-Edges    ");           //NOLINT
    xlib::printArray(_out_degrees, _nV,     "Out-Degrees  ");           //NOLINT
    if (_structure.is_directed() && _structure.is_reverse()) {
        xlib::printArray(_in_offsets, _nV + 1, "In-Offsets   ");        //NOLINT
        xlib::printArray(_in_edges,   _nE,     "In-Edges     ");        //NOLINT
        xlib::printArray(_in_degrees, _nV,     "In-Degrees   ");        //NOLINT
    }
}

#if defined(__linux__)

template<typename vid_t, typename eoff_t>
void GraphStd<vid_t, eoff_t>
::writeBinary(const std::string& filename, bool print) const {
    using namespace structure_prop;
    size_t  base_size = sizeof(_nV) + sizeof(_nE) + sizeof(_structure);
    size_t file_size1 = (static_cast<size_t>(_nV) + 1) * sizeof(eoff_t) +
                        (static_cast<size_t>(_nE)) * sizeof(vid_t);

    bool       twice = _structure.is_directed() && _structure.is_reverse();
    size_t file_size = base_size + (twice ? file_size1 * 2 : file_size1);

    if (print) {
        std::cout << "Graph to binary file: " << filename
                << " (" << (file_size >> 20) << ") MB" << std::endl;
    }

    std::string class_id = xlib::type_name<vid_t>() + xlib::type_name<eoff_t>();
    file_size           += class_id.size();
    xlib::MemoryMapped memory_mapped(filename.c_str(), file_size,
                                     xlib::MemoryMapped::WRITE, print);

    if (_structure.is_directed() && _structure.is_reverse()) {
        auto struct_tmp = DIRECTED | REVERSE;
        memory_mapped.write(class_id.c_str(), class_id.size(),          //NOLINT
                            &_nV, 1, &_nE, 1, &struct_tmp, 1,           //NOLINT
                            _out_offsets, _nV + 1, _in_offsets, _nV + 1,//NOLINT
                            _out_edges, _nE, _in_edges, _nE);           //NOLINT
    }
    else {
        auto struct_tmp = DIRECTED;
        memory_mapped.write(class_id.c_str(), class_id.size(),          //NOLINT
                            &_nV, 1, &_nE, 1, &struct_tmp, 1,           //NOLINT
                            _out_offsets, _nV + 1, _out_edges, _nE);    //NOLINT
    }
}

#pragma clang diagnostic pop
#endif

template<typename vid_t, typename eoff_t>
void GraphStd<vid_t, eoff_t>::writeMarket(const std::string& filename,
                                          bool print) const {
    if (print)
        std::cout << "Graph to Market format file: " << filename << std::endl;
    std::ofstream fout(filename);
    fout << "%%MatrixMarket matrix coordinate pattern general\n"
         << _nV << " " << _nV << " " << _nE << "\n";
    for (vid_t i = 0; i < _nV; i++) {
        for (auto j = _out_offsets[i]; j < _out_offsets[i + 1]; j++)
            fout << i + 1 << " " << _out_edges[j] + 1 << "\n";
    }
    fout.close();
}

template<typename vid_t, typename eoff_t>
void GraphStd<vid_t, eoff_t>::writeDimacs10th(const std::string& filename,
                                              bool print) const {
    if (print) {
        std::cout << "Graph to Dimacs10th format file: " << filename
                  << std::endl;
    }
    std::ofstream fout(filename);
    fout << _nV << " " << _nE << " 100\n";
    for (vid_t i = 0; i < _nV; i++) {
        for (auto j = _out_offsets[i]; j < _out_offsets[i + 1]; j++) {
            fout << _out_edges[j] + 1;
            if (j < _out_offsets[i + 1] - 1)
                fout << " ";
        }
        fout << "\n";
    }
    fout.close();
}
//------------------------------------------------------------------------------

template<typename vid_t, typename eoff_t>
void GraphStd<vid_t, eoff_t>::print_degree_distrib() const noexcept {
    const int MAX_LOG = 32;
    int distribution[MAX_LOG] = {};
    int   cumulative[MAX_LOG] = {};
    int      percent[MAX_LOG];
    int cumulative_percent[MAX_LOG];
    for (auto i = 0; i < _nV; i++) {
        auto degree = _out_degrees[i];
        if (degree == 0) continue;
        auto log_value = xlib::log2(degree);
        distribution[log_value]++;
        cumulative[log_value] += degree;
    }
    for (auto i = 0; i < MAX_LOG; i++) {
        percent[i] = xlib::per_cent(distribution[i], _nV);
        cumulative_percent[i] = xlib::per_cent(cumulative[i], _nE);
    }
    int sum = 0;
    for (auto i = 0; i < MAX_LOG; i++)
        sum += cumulative[i];
    std::cout << "sum  " << sum << std::endl;

    int pos = MAX_LOG;
    while (pos >= 0 && distribution[--pos] == 0);

    xlib::IosFlagSaver tmp1;
    xlib::ThousandSep  tmp2;
    using namespace std::string_literals;

    std::cout << "Degree distribution:" << std::setprecision(1) << "\n\n";
    for (auto i = 0; i <= pos; i++) {
        std::string exp = "  (2^"s + std::to_string(i) + ")"s;
        std::cout << std::right << std::setw(9)  << (1 << i)
                  << std::left  << std::setw(8)  << exp
                  << std::right << std::setw(12) << distribution[i]
                  << std::right << std::setw(5)  << percent[i] << " %\n";
    }
    std::cout << "\nEdge distribution:" << std::setprecision(1) << "\n\n";
    for (auto i = 0; i <= pos; i++) {
        std::string exp = "  (2^"s + std::to_string(i) + ")"s;
        std::cout << std::right << std::setw(9)  << (1 << i)
                  << std::left  << std::setw(8)  << exp
                  << std::right << std::setw(12) << cumulative[i]
                  << std::right << std::setw(5)  << cumulative_percent[i]
                  << " %\n";
    }
    std::cout << std::endl;
}

template<typename vid_t, typename eoff_t>
void GraphStd<vid_t, eoff_t>::print_degree_analysis() const noexcept {
    auto avg      = static_cast<float>(_nE) / static_cast<float>(_nV);
    auto std_dev  = xlib::std_deviation(_out_degrees, _out_degrees + _nV);
    auto density  = static_cast<float>(_nE) /
                       static_cast<float>(static_cast<uint64_t>(_nV) * _nV);
    auto gini     = xlib::gini_coefficient(_out_degrees, _out_degrees + _nV);
    auto variance_coeff = std_dev / std::abs(avg);
    xlib::Bitmask rings(_nV);
    for (auto i = 0; i < _nV; i++) {
        for (auto j = _out_offsets[i]; j < _out_offsets[i + 1]; j++) {
            if (_out_edges[j] == i) {
                rings[i] = true;
                break;
            }
        }
    }
    int     num_rings = rings.size();
    auto ring_percent = xlib::per_cent(rings.size(), _nV);

    degree_t out_degree_0 = 0, in_degree_0 = 0, out_degree_1 = 0,
              in_degree_1 = 0,   singleton = 0,     out_leaf = 0, in_leaf = 0;
    auto max_out_degree = std::numeric_limits<degree_t>::min();
    auto  max_in_degree = std::numeric_limits<degree_t>::min();
    for (auto i = 0; i < _nV; i++) {
        if (_out_degrees[i] > max_out_degree)
            max_out_degree = _out_degrees[i];
        if (_in_degrees[i] > max_in_degree)
            max_in_degree = _in_degrees[i];
        if (_out_degrees[i] == 0) out_degree_0++;
        if (_out_degrees[i] == 1) out_degree_1++;
        if (_in_degrees[i] == 0)  in_degree_0++;
        if (_in_degrees[i] == 1)  in_degree_1++;
        if ((_out_degrees[i] == 0 && _in_degrees[i] == 0) ||
            (_out_degrees[i] == 1 && _in_degrees[i] == 1 && rings[i]))
            singleton++;
        if (((_out_degrees[i] == 2 && is_undirected()) ||
            (_out_degrees[i] == 1 && is_directed())) && rings[i])
            out_leaf++;
        if (((_in_degrees[i] == 2 && is_undirected()) ||
            (_in_degrees[i] == 1 && is_directed())) && rings[i])
            in_leaf++;
    }
    auto out_degree_0_percent = xlib::per_cent(out_degree_0, _nV);
    auto out_degree_1_percent = xlib::per_cent(out_degree_1, _nV);
    auto in_degree_0_percent  = xlib::per_cent(in_degree_0, _nV);
    auto in_degree_1_percent  = xlib::per_cent(in_degree_1, _nV);
    auto singleton_percent    = xlib::per_cent(singleton, _nV);
    auto out_leaf_percent     = xlib::per_cent(out_leaf, _nV);
    auto in_leaf_percent      = xlib::per_cent(in_leaf, _nV);

    const int W1 = 30;
    const int W2 = 10;
    const int W3 = 8;
    xlib::IosFlagSaver tmp1;
    xlib::ThousandSep tmp2;

    std::cout << "Degree analysis:"
    << std::right << std::setprecision(1) << std::fixed << "\n\n"
    << std::setw(W1) << "Average:  "             << std::setw(W2) << avg << "\n"
    << std::setw(W1) << "Std. Deviation:  "      << std::setw(W2) << std_dev
                                                 << "\n"
    << std::setw(W1) << "Coeff. of variation:  " << std::setw(W2)
                                                 << variance_coeff  << "\n"
    << std::setw(W1) << "Gini Coeff:  "          << std::setprecision(2)
                                                 << std::setw(W2)
                                                 << gini  << "\n"
    << std::setw(W1) << "Density:  "             << std::setprecision(7)
                                                 << std::setw(W2)
                                                 << density  << "\n"
                                                 << std::setprecision(1)

    << std::setw(W1) << "Max Out-Degree:  "  << std::setw(W2) << max_out_degree
                                             << "\n"
    << std::setw(W1) << "Max In-Degree:  "   << std::setw(W2) << max_in_degree
                                             << "\n"
    << std::setw(W1) << "Rings:  "           << std::setw(W2) << num_rings
                                             << std::setw(W3) << ring_percent
                                             << "%\n"
    << std::setw(W1) << "Out-Degree = 0:  "  << std::setw(W2) << out_degree_0
                                             << std::setw(W3)
                                             << out_degree_0_percent << "%\n";
    if (is_directed()) {
        std::cout << std::setw(W1) << "In-Degree = 0:  "
                                             << std::setw(W2) << in_degree_0
                                             << std::setw(W3)
                                             << in_degree_0_percent << "%\n";
    }
    std::cout
    << std::setw(W1) << "Out-Degree = 1:  "  << std::setw(W2) << out_degree_1
                                             << std::setw(W3)
                                             << out_degree_1_percent << "%\n";
    if (is_directed()) {
        std::cout << std::setw(W1) << "In-Degree = 1:  "
                                             << std::setw(W2) << in_degree_1
                                             << std::setw(W3)
                                             << in_degree_1_percent << "%\n";
    }

    std::cout << std::setw(W1) << "leaf:  "  << std::setw(W2) << singleton
                                             << std::setw(W3)
                                             << singleton_percent << "%\n"
    << std::setw(W1) << "Out-Leaf:  "        << std::setw(W2) << out_leaf
                                             << std::setw(W3)
                                             << out_leaf_percent << "%\n"
    << std::setw(W1) << "In-Leaf:  "         << std::setw(W2) << in_leaf
                                             << std::setw(W3)
                                             << in_leaf_percent << "%\n"
                                             << std::endl;
}

//------------------------------------------------------------------------------

template class GraphStd<int, int>;
template class GraphStd<int64_t, int64_t>;

} // namespace graph
