﻿#pragma once

#include "opengl.hpp"
#include <vector>
#include <tuple>
#include <utility>

namespace dgl {


    template<class T>
    class structured_buffer;

    template<class T>
    class void_buffer: public base {
    protected:
        using buffer = void_buffer<T>;

    public:
        void_buffer<T>(GLuint * allocationPointer) { this->set_object(*allocationPointer); } // can be used with allocators
        void_buffer<T>() { base::allocate(1); glCreateBuffers(1, thisref); }
        ~void_buffer<T>() { glDeleteBuffers(1, thisref); this->set_value(-1); }

        void get_subdata(GLintptr offset, GLsizei size, void *data) const {
            glGetNamedBufferSubData(thisref, offset, size, data);
        }

        void data(GLsizei size, const void *data, GLenum usage = GL_STATIC_DRAW){
            glNamedBufferData(thisref, size, data, usage);
        }

        void subdata(GLintptr offset, GLsizei size, const void *data){
            glNamedBufferSubData(thisref, offset, size, data);
        }

        void storage(GLsizei size, const void *data = nullptr, buffer_storage_bits flags = GL_DYNAMIC_STORAGE_BIT) {
            glNamedBufferStorage(thisref, size, data, flags.bitfield);
        }

        void copydata(void_buffer<T>& dest, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size){
            glCopyNamedBufferSubData(thisref, *dest, readOffset, writeOffset, size);
        }
    };

    template<class T>
    class structured_buffer: public void_buffer<T> {
    protected:
        using buffer = void_buffer<T>;

        // create tuple of buffers
        template<typename... T, size_t... Is>
        static decltype(auto) _make_tuple(GLuint * a, std::index_sequence<Is...>)
        {
            //return std::make_tuple(structured_buffer<T>(a + Is)...); // need construct with `new`, because C++ removes classes outside of this scope
            return std::make_tuple(*(new structured_buffer<T>(a + Is))...);
        }

    public:
        
        structured_buffer<T>(GLuint * allocationPointer) : void_buffer<T>(allocationPointer) {};
        structured_buffer<T>() : void_buffer<T>() {};

        // new multi-bind creator
        static decltype(auto) create(GLint n) {
            GLuint * objects = new GLuint[n];
            std::vector<structured_buffer<T>> buffers;
            for (intptr_t pt = 0; pt < n; pt++) {
                buffers.push_back(structured_buffer<T>(objects + pt));
            }
            glCreateBuffers(n, objects);
            return buffers;
        }

        template<typename... T>
        static decltype(auto) create() {
            constexpr size_t n = sizeof...(T);
            GLuint * objects = new GLuint[n];
            glCreateBuffers(n, objects);
            return _make_tuple<T...>(objects, std::index_sequence_for<T...>{});
        }


        void get_subdata(GLintptr offset, GLsizei size, void *data) const {
            buffer::get_subdata(offset, size * sizeof(T), data);
        }

        void data(GLsizei size, const void *data, GLenum usage = GL_STATIC_DRAW){
            buffer::data(size * sizeof(T), data, usage);
        }

        void subdata(GLintptr offset, GLsizei size, const void *data){
            buffer::subdata(offset, size * sizeof(T), data);
        }


        void storage(GLsizei size, const void *data = nullptr, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT){
            buffer::storage(size * sizeof(T), data, flags);
        }

        void copydata(buffer& dest, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size){
            buffer::copydata(dest, readOffset, writeOffset, size * sizeof(T));
        }


        void data(const std::vector<T>& data, GLenum usage = GL_STATIC_DRAW){
            return this->data<T>(data, usage);
        }

        void subdata(GLintptr offset, const std::vector<T>& data){
            return this->subdata<T>(offset, data);
        }

        std::vector<T>& get_subdata(GLintptr offset, GLsizei size) const {
            return this->get_subdata<T>(offset, size);
        }

        std::vector<T>& get_subdata(GLintptr offset, std::vector<T>&vctr) const {
            return this->get_subdata<T>(offset, vctr);
        }






        // data from vector
        template<class T>
        void data(const std::vector<T>& data, GLenum usage = GL_STATIC_DRAW) {
            buffer::data(data.size() * sizeof(T), data.data(), usage);
        }

        // fill subdata by vector
        template<class T>
        void subdata(GLintptr offset, const std::vector<T>& data) {
            buffer::subdata(offset, data.size() * sizeof(T), data.data());
        }

        // get subdata by range
        template<class T>
        std::vector<T>& get_subdata(GLintptr offset, GLsizei size) const {
            std::vector<T> vctr(size);
            buffer::get_subdata(offset, vctr.size() * sizeof(T), vctr.data());
            return vctr;
        }

        // get subdata by full vector
        template<class T>
        std::vector<T>& get_subdata(GLintptr offset, std::vector<T>&vctr) const {
            buffer::get_subdata(offset, vctr.size() * sizeof(T), vctr.data());
            return vctr;
        }


    };


    // buffer is structured_buffer<void>;
    using buffer = structured_buffer<GLubyte>;



    class _buffer_context;

    // register buffer binding
    class buffer_binding: public base {
    protected:
        friend _buffer_context;
        _buffer_context * gltarget;

    public:
        buffer_binding(_buffer_context& btarget, GLuint binding = 0) {
            base::allocate(1);
            gltarget = &btarget; 
            this->set_value(binding); 
        }

        ~buffer_binding();

        template<typename T>
        void bind(structured_buffer<T>& buf);

        template<typename T>
        void bind_range(structured_buffer<T>& buf, GLintptr offset = 0, GLsizei size = 1);

        template<typename T>
        void bind(std::vector<structured_buffer<T>>& buf);

        template<typename T>
        void bind_range(std::vector<structured_buffer<T>>& buf, GLintptr * offsets = nullptr, GLsizeiptr * sizes = nullptr);
    };


    // contextual targeted bindings
    class _buffer_context: public base {
    public:
        _buffer_context(GLuint binding = 0) {
            base::allocate(1);
            this->set_value(binding);
        }

        buffer_binding&& create_binding(GLuint binding = 0){
            return buffer_binding(thisref, binding);
        }

        // context named binding
        void bind(buffer& buf){
            glBindBuffer(thisref, buf);
        }


        // TODO support of non-DSA (binded) operations
    };


    
    buffer_binding:: ~buffer_binding() { // unbind
        glBindBufferBase(*gltarget, thisref, 0); 
    }


    // basic bind support
    template<typename T>
    void buffer_binding::bind(structured_buffer<T>& buf) {
        glBindBufferBase(*gltarget, thisref, buf);
    }

    template<typename T>
    void buffer_binding::bind_range(structured_buffer<T>& buf, GLintptr offset, GLsizei size) {
        glBindBufferRange(*gltarget, thisref, buf, offset, size);
    }

    // multi-bind support
    template<typename T>
    void buffer_binding::bind(std::vector<structured_buffer<T>>& buf) {
        GLuint handler = buf[0];
        glBindBuffersBase(*gltarget, thisref, buf.size(), /*buf.data()*/ &handler);
    }

    template<typename T>
    void buffer_binding::bind_range(std::vector<structured_buffer<T>>& buf, GLintptr * offsets, GLsizeiptr * sizes) {
        GLuint handler = buf[0];
        glBindBuffersRange(*gltarget, thisref, buf.size(), &handler, offsets, sizes);
    }





    // crop tuple element
    template <class Tm, class... T, size_t... Is>
    constexpr decltype(auto) get_stride_reduce_impl(GLsizei * rest, std::tuple<structured_buffer<Tm>, structured_buffer<T>...> &t, std::index_sequence<Is...>) {
        *rest = (GLsizei)sizeof(typename std::tuple_element<0, std::tuple<T...>>::type); return std::make_tuple(std::get<Is + 1>(t)...);
    }

    template <class... T>
    constexpr decltype(auto) get_stride_reduce(GLsizei * rest, std::tuple<structured_buffer<T>...> &t) {
        return get_stride_reduce_impl(rest, t, std::make_index_sequence<(sizeof...(T)) - 1>{});
    }

    // getting strides of buffer types
    template <class T>
    constexpr decltype(auto) get_stride_impl(GLsizei * ptr, std::tuple<structured_buffer<T>> &t) {
        *ptr = sizeof(typename std::tuple_element<0, std::tuple<T>>::type);
    }

    template <class... T>
    constexpr decltype(auto) get_stride_impl(GLsizei * ptr, std::tuple<structured_buffer<T>...> &t) {
        decltype(auto) rest_tuple = get_globj_reduce<T...>(ptr, t);
        get_stride_impl(ptr + 1, rest_tuple);
    }

    template <class... T>
    constexpr decltype(auto) get_stride(GLsizei * ptr, std::tuple<structured_buffer<T>...> &t) {
        return get_stride_impl(ptr, t);
    }




    // bindables
    namespace buffer_target {
        _buffer_context element_array(GL_ELEMENT_ARRAY_BUFFER);
        _buffer_context array(GL_ARRAY_BUFFER);
        _buffer_context shader_storage(GL_SHADER_STORAGE_BUFFER);
        _buffer_context uniform(GL_UNIFORM_BUFFER);
    }

};