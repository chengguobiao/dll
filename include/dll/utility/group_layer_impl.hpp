//=======================================================================
// Copyright (c) 2014-2017 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#pragma once

#include "dll/neural_layer.hpp"

#include "dll/util/timers.hpp" // for auto_timer

namespace dll {

/*!
 * \brief Standard convolutional layer of neural network.
 */
template <typename... Layers>
struct group_layer_impl final : layer<group_layer_impl<Layers...>> {
    using first_layer_t = cpp::first_type_t<Layers...>; ///< The type of the first layer
    using last_layer_t  = cpp::last_type_t<Layers...>;  ///< The type of the last layer

    using this_type = group_layer_impl<Layers...>;    ///< The type of this layer
    using weight    = typename first_layer_t::weight; ///< The data type of the layer
    using base_type = layer<this_type>;               ///< The base type of the layer

    using input_one_t  = typename first_layer_t::input_one_t; ///< The type of one input
    using output_one_t = typename last_layer_t::output_one_t; ///< The type of one output
    using input_t      = std::vector<input_one_t>;            ///< The type of the input
    using output_t     = std::vector<output_one_t>;           ///< The type of the output

    static constexpr size_t n_layers = sizeof...(Layers); ///< The number of layers

    std::tuple<Layers...> layers;

    /*!
     * \brief Initialize a conv layer with basic weights.
     */
    group_layer_impl() = default;

    group_layer_impl(group_layer_impl& rhs) = delete;
    group_layer_impl& operator=(const group_layer_impl& rhs) = delete;

    group_layer_impl(const group_layer_impl&& rhs) = delete;
    group_layer_impl& operator=(const group_layer_impl&& rhs) = delete;

    /*!
     * \brief Return the size of the input of this layer
     * \return The size of the input of this layer
     */
    static constexpr size_t input_size() noexcept {
        return first_layer_t::input_size();
    }

    /*!
     * \brief Return the size of the output of this layer
     * \return The size of the output of this layer
     */
    static constexpr size_t output_size() noexcept {
        return last_layer_t::output_size();
    }

    /*!
     * \brief Return the number of trainable parameters of this network.
     * \return The the number of trainable parameters of this network.
     */
    static constexpr size_t parameters() noexcept {
        // TODO Fix this!
        return last_layer_t::parameters();
    }

    /*!
     * \brief Returns a short description of the layer
     * \return an std::string containing a short description of the layer
     */
    std::string to_short_string() const {
        std::string str = "Group(";

        cpp::for_each(layers, [&str](auto& layer){
            str += "\n  " + layer.to_short_string();
        });

        str += "\n)";

        return str;
    }

    using base_type::forward_batch;
    using base_type::train_forward_batch;
    using base_type::test_forward_batch;

    template <size_t L, typename H1, typename V, cpp_enable_iff(L != n_layers - 1)>
    void test_forward_batch_sub(H1&& output, const V& input) const {
        auto next_input = std::get<L>(layers).test_forward_batch(input);

        test_forward_batch_sub<L+1>(output, next_input);
    }

    template <size_t L, typename H1, typename V, cpp_enable_iff(L == n_layers - 1)>
    void test_forward_batch_sub(H1&& output, const V& input) const {
        std::get<L>(layers).test_forward_batch(output, input);
    }

    /*!
     * \brief Apply the layer to the given batch of input.
     *
     * \param input A batch of input
     * \param output A batch of output that will be filled
     */
    template <typename H1, typename V>
    void test_forward_batch(H1&& output, const V& input) const {
        test_forward_batch_sub<0>(output, input);
    }

    template <size_t L, typename H1, typename V, cpp_enable_iff(L != n_layers - 1)>
    void train_forward_batch_sub(H1&& output, const V& input) const {
        auto next_input = std::get<L>(layers).train_forward_batch(input);

        train_forward_batch_sub<L+1>(output, next_input);
    }

    template <size_t L, typename H1, typename V, cpp_enable_iff(L == n_layers - 1)>
    void train_forward_batch_sub(H1&& output, const V& input) const {
        std::get<L>(layers).train_forward_batch(output, input);
    }

    /*!
     * \brief Apply the layer to the given batch of input.
     *
     * \param input A batch of input
     * \param output A batch of output that will be filled
     */
    template <typename H1, typename V>
    void train_forward_batch(H1&& output, const V& input) const {
        train_forward_batch_sub<0>(output, input);
    }

    template <size_t L, typename H1, typename V, cpp_enable_iff(L != n_layers - 1)>
    void forward_batch_sub(H1&& output, const V& input) const {
        auto next_input = std::get<L>(layers).forward_batch(input);

        forward_batch_sub<L+1>(output, next_input);
    }

    template <size_t L, typename H1, typename V, cpp_enable_iff(L == n_layers - 1)>
    void forward_batch_sub(H1&& output, const V& input) const {
        std::get<L>(layers).forward_batch(output, input);
    }

    /*!
     * \brief Apply the layer to the given batch of input.
     *
     * \param input A batch of input
     * \param output A batch of output that will be filled
     */
    template <typename H1, typename V>
    void forward_batch(H1&& output, const V& input) const {
        forward_batch_sub<0>(output, input);
    }

    /*!
     * \brief Prepare one empty output for this layer
     * \return an empty ETL matrix suitable to store one output of this layer
     */
    template <typename Input>
    output_one_t prepare_one_output() const {
        return {};
    }

    /*!
     * \brief Prepare a set of empty outputs for this layer
     * \param samples The number of samples to prepare the output for
     * \return a container containing empty ETL matrices suitable to store samples output of this layer
     */
    template <typename Input>
    static output_t prepare_output(size_t samples) {
        return output_t{samples};
    }

    /*!
     * \brief Initialize the dynamic version of the layer from the
     * fast version of the layer
     * \param dyn Reference to the dynamic version of the layer that
     * needs to be initialized
     */
    template<typename DRBM>
    static void dyn_init(DRBM& dyn){
        //TODO
        //dyn.init_layer(NC, NV1, NV2, K, NW1, NW2);
    }

    /*!
     * \brief Backup the weights in the secondary weights matrix
     */
    void backup_weights() {
        cpp::for_each(layers, [](auto& layer) {
            cpp::static_if<decay_layer_traits<decltype(layer)>::is_trained()>([&layer](auto f) {
                f(layer).backup_weights();
            });
        });
    }

    /*!
     * \brief Restore the weights from the secondary weights matrix
     */
    void restore_weights() {
        cpp::for_each(layers, [](auto& layer){
            cpp::static_if<decay_layer_traits<decltype(layer)>::is_trained()>([&layer](auto f) {
                f(layer).restore_weights();
            });
        });
    }
};

// Declare the traits for the Layer

template<typename... Layers>
struct layer_base_traits<group_layer_impl<Layers...>> {
    static constexpr bool is_neural     = true;  ///< Indicates if the layer is a neural layer
    static constexpr bool is_dense      = false; ///< Indicates if the layer is dense
    static constexpr bool is_conv       = false; ///< Indicates if the layer is convolutional
    static constexpr bool is_deconv     = false; ///< Indicates if the layer is deconvolutional
    static constexpr bool is_standard   = true;  ///< Indicates if the layer is standard
    static constexpr bool is_rbm        = false; ///< Indicates if the layer is RBM
    static constexpr bool is_pooling    = false; ///< Indicates if the layer is a pooling layer
    static constexpr bool is_unpooling  = false; ///< Indicates if the layer is an unpooling laye
    static constexpr bool is_transform  = false; ///< Indicates if the layer is a transform layer
    static constexpr bool is_dynamic    = false; ///< Indicates if the layer is dynamic
    static constexpr bool pretrain_last = false; ///< Indicates if the layer is dynamic
    static constexpr bool sgd_supported = true;  ///< Indicates if the layer is supported by SGD
};

} //end of dll namespace
