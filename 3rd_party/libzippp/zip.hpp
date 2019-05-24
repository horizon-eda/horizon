/*
 * zip.hpp -- a safe modern C++ wrapper on top of libzip
 *
 * Copyright (c) 2013-2018 David Demelier <markand@malikania.fr>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef ZIP_HPP
#define ZIP_HPP

/**
 * \file zip.hpp
 * \brief A safe modern C++ wrapper on top of libzip.
 * \author David Demelier <markand@malikania.fr>
 * \version 2.0.0-dev
 */

#if defined(_MSC_VER)
#   if !defined(_CRT_SECURE_NO_WARNINGS)
#       define _CRT_SECURE_NO_WARNINGS
#   endif
#endif

#include <cassert>
#include <cerrno>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <string>

#include <zip.h>

/**
 * \brief The libzip namespace.
 */
namespace libzip {

/**
 * \brief zip_stat typedef.
 */
typedef struct zip_stat stat;

/**
 * \brief zip_source_cmd typedef.
 */
using source_cmd = enum zip_source_cmd;

/**
 * \brief zip_source_cmd typedef.
 */
using source_callback = zip_source_callback;

/**
 * \brief zip_flags_t typedef.
 */
using flags_t = zip_flags_t;

/**
 * \brief zip_int8_t typedef.
 */
using int8_t = zip_int8_t;

/**
 * \brief zip_uint8_t typedef.
 */
using uint8_t = zip_uint8_t;

/**
 * \brief zip_int16_t typedef.
 */
using int16_t = zip_int16_t;

/**
 * \brief zip_uint16_t typedef.
 */
using uint16_t = zip_uint16_t;

/**
 * \brief zip_int32_t typedef.
 */
using int32_t = zip_int32_t;

/**
 * \brief zip_uint32_t typedef.
 */
using uint32_t = zip_uint32_t;

/**
 * \brief zip_int64_t typedef.
 */
using int64_t = zip_int64_t;

/**
 * \brief zip_uint64_t_t typedef.
 */
using uint64_t = zip_uint64_t;

/**
 * \brief Source creation for adding files.
 *
 * This function must be passed through archive::add, it will be called to
 * create the appropriate zip_source  * inside the function.
 *
 * The user should not call the function itself as it's the responsability of
 * libzip to destroy the zip_source structure.
 *
 * \see source::buffer
 * \see source::file
 */
using source = std::function<struct zip_source* (struct zip*)>;

/**
 * \brief File for reading.
 */
class file {
private:
    std::unique_ptr<struct zip_file, int (*)(struct zip_file*)> handle_;

    file(const file&) = delete;
    file& operator=(const file&) = delete;

public:
    /**
     * Create a File with a zip_file structure.
     *
     * \param file the file ready to be used
     */
    inline file(struct zip_file* file) noexcept
        : handle_(file, zip_fclose)
    {
    }

    /**
     * Move constructor defaulted.
     *
     * \param other the other File
     */
    file(file&& other) noexcept = default;

    /**
     * Move operator defaulted.
     *
     * \return *this
     */
    file& operator=(file&&) noexcept = default;

    /**
     * Read some data.
     *
     * \param data the destination buffer
     * \param length the length
     * \return the number of bytes written or -1 on failure
     */
    inline int read(void* data, uint64_t length) noexcept
    {
        return zip_fread(handle_.get(), data, length);
    }

    /**
     * Read some data to a fixed size array.
     *
     * \param data the array
     * \return the number of bytes written or -1 on failure
     */
    template <size_t Size>
    inline int read(char (&data)[Size]) noexcept
    {
        return read(data, Size);
    }

    /**
     * Optimized function for reading all characters with only one allocation.
     * Ideal for combining with archive::stat.
     *
     * \param length the length of the file
     * \return the whole string
     * \see archive::stat
     */
    std::string read(uint64_t length)
    {
        std::string result;

        result.resize(length);
        auto count = read(&result[0], length);

        if (count < 0)
            return "";

        result.resize(count);

        return result;
    }
};

/**
 * Add a file to the archive using a binary buffer.
 *
 * \param data the buffer
 * \return the source to add
 */
inline source source_buffer(std::string data) noexcept
{
    return [data = std::move(data)] (struct zip* archive) -> struct zip_source* {
        auto size = data.size();
        auto ptr = static_cast<char*>(std::malloc(size));

        if (ptr == nullptr)
            throw std::runtime_error(std::strerror(errno));

        std::memcpy(ptr, data.data(), size);

        auto src = zip_source_buffer(archive, ptr, size, 1);

        if (src == nullptr) {
            std::free(ptr);
            throw std::runtime_error(zip_strerror(archive));
        }

        return src;
    };
}

/**
 * Add a file to the archive from the disk.
 *
 * \param path the path to the file
 * \param start the position where to start
 * \param length the number of bytes to copy
 * \return the source to add
 */
inline source source_file(std::string path, uint64_t start = 0, int64_t length = -1) noexcept
{
    return [path = std::move(path), start, length] (struct zip* archive) -> struct zip_source* {
        auto src = zip_source_file(archive, path.c_str(), start, length);

        if (src == nullptr)
            throw std::runtime_error(zip_strerror(archive));

        return src;
    };
}

/**
 * \brief Wrapper for stat as pointer.
 */
class stat_ptr {
private:
    stat stat_;

public:
    /**
     * Constructor.
     *
     * \param stat the file stat
     */
    inline stat_ptr(stat stat) noexcept
        : stat_(stat)
    {
    }

    /**
     * Get the reference.
     *
     * \return the reference
     */
    inline stat& operator*() noexcept
    {
        return stat_;
    }

    /**
     * Get the reference.
     *
     * \return the reference
     */
    inline const stat& operator*() const noexcept
    {
        return stat_;
    }

    /**
     * Access the object.
     *
     * \return the pointer
     */
    inline stat* operator->() noexcept
    {
        return &stat_;
    }

    /**
     * Access the object.
     *
     * \return the pointer
     */
    inline const stat* operator->() const noexcept
    {
        return &stat_;
    }
};

/**
 * \brief Safe wrapper on the struct zip structure.
 */
class archive {
private:
    std::unique_ptr<struct zip, int (*)(struct zip *)> handle_;

    archive(const archive&) = delete;
    archive& operator=(const archive&) = delete;

public:
    /**
     * \brief Base iterator class
     */
    class iterator : public std::iterator<std::random_access_iterator_tag, libzip::stat> {
    private:
        friend class archive;

        archive *archive_{nullptr};
        uint64_t index_{0};

        inline iterator(archive* archive, uint64_t index = 0) noexcept
            : archive_(archive)
            , index_(index)
        {
        }

    public:
        /**
         * Default iterator.
         */
        iterator() noexcept = default;

        /**
         * Dereference the iterator.
         *
         * \return the stat information
         */
        inline libzip::stat operator*() const
        {
            assert(archive_);

            return archive_->stat(index_);
        }

        /**
         * Dereference the iterator.
         *
         * \return the stat information as point
         */
        inline stat_ptr operator->() const
        {
            assert(archive_);

            return stat_ptr(archive_->stat(index_));
        }

        /**
         * Post increment.
         *
         * \return this
         */
        inline iterator& operator++() noexcept
        {
            ++ index_;

            return *this;
        }

        /**
         * Pre increment.
         *
         * \return this
         */
        inline iterator operator++(int) noexcept
        {
            iterator save = *this;

            ++ index_;

            return save;
        }

        /**
         * Post decrement.
         *
         * \return this
         */
        inline iterator& operator--() noexcept
        {
            -- index_;

            return *this;
        }

        /**
         * Pre decrement.
         *
         * \return this
         */
        inline iterator operator--(int) noexcept
        {
            iterator save = *this;

            -- index_;

            return save;
        }

        /**
         * Increment.
         *
         * \param inc the number
         * \return the new iterator
         */
        inline iterator operator+(int inc) const noexcept
        {
            return iterator(archive_, index_ + inc);
        }

        /**
         * Decrement.
         *
         * \param dec the number
         * \return the new iterator
         */
        inline iterator operator-(int dec) const noexcept
        {
            return iterator(archive_, index_ - dec);
        }

        /**
         * Compare equality.
         *
         * \param other the other iterator
         * \return true if same
         */
        inline bool operator==(const iterator& other) const noexcept
        {
            return index_ == other.index_;
        }

        /**
         * Compare equality.
         *
         * \param other the other iterator
         * \return true if different
         */
        inline bool operator!=(const iterator& other) const noexcept
        {
            return index_ != other.index_;
        }

        /**
         * Access a stat information at the specified index.
         *
         * \pre must not be default-constructed
         * \param index the new index
         * \return stat information
         * \throw std::runtime_error on errors
         */
        inline libzip::stat operator[](int index)
        {
            assert(archive_);

            return archive_->stat(index_ + index);
        }
    };

public:
    /**
     * Iterator conversion to Stat.
     */
    using value_type = libzip::stat;

    /**
     * Reference is a copy of Stat.
     */
    using reference = libzip::stat;

    /**
     * Const reference is a copy of Stat.
     */
    using const_reference = libzip::stat;

    /**
     * Pointer is a small wrapper
     */
    using pointer = stat_ptr;

    /**
     * Type of difference.
     */
    using size_type = unsigned;

    /**
     * Const random access iterator.
     */
    using const_iterator = iterator;

public:
    /**
     * Open an archive on the disk.
     *
     * \param path the path
     * \param flags the optional flags
     * \throw std::runtime_error on errors
     */
    archive(const std::string& path, flags_t flags = 0)
        : handle_(nullptr, nullptr)
    {
        int error;
        struct zip* archive = zip_open(path.c_str(), flags, &error);

        if (archive == nullptr) {
            char buf[128]{0};

            zip_error_to_str(buf, sizeof (buf), error, errno);

            throw std::runtime_error(buf);
        }

        handle_ = { archive, zip_close };
    }

    /**
     * Move constructor defaulted.
     *
     * \param other the other archive
     */
    archive(archive&& other) noexcept = default;

    /**
     * Move operator defaulted.
     *
     * \param other the other archive
     * \return *this
     */
    archive& operator=(archive&& other) noexcept = default;

    /**
     * Get an iterator to the beginning.
     *
     * \return the iterator
     */
    inline iterator begin() noexcept
    {
        return iterator(this);
    }

    /**
     * Overloaded function.
     *
     * \return the iterator
     */
    inline const_iterator begin() const noexcept
    {
        return const_iterator(const_cast<archive*>(this));
    }

    /**
     * Overloaded function.
     *
     * \return the iterator
     */
    inline const_iterator cbegin() const noexcept
    {
        return const_iterator(const_cast<archive*>(this));
    }

    /**
     * Get an iterator to the end.
     *
     * \return the iterator
     */
    inline iterator end() noexcept
    {
        return iterator(this, num_entries());
    }

    /**
     * Overloaded function.
     *
     * \return the iterator
     */
    inline const_iterator end() const noexcept
    {
        return const_iterator(const_cast<archive*>(this), num_entries());
    }

    /**
     * Overloaded function.
     *
     * \return the iterator
     */
    inline const_iterator cend() const noexcept
    {
        return const_iterator(const_cast<archive*>(this), num_entries());
    }

    /**
     * Set a comment on a file.
     *
     * \param index the file index in the archive
     * \param text the text or empty to remove the comment
     * \param flags the optional flags
     * \throw std::runtime_error on errors
     */
    void set_file_comment(uint64_t index, const std::string& text = "", flags_t flags = 0)
    {
        auto size = text.size();
        auto cstr = (size == 0) ? nullptr : text.c_str();

        if (zip_file_set_comment(handle_.get(), index, cstr, size, flags) < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));
    }

    /**
     * Get a comment from a file.
     *
     * \param index the file index in the archive
     * \param flags the optional flags
     * \return the comment
     * \throw std::runtime_error on errors
     */
    std::string file_comment(uint64_t index, flags_t flags = 0) const
    {
        zip_uint32_t length = 0;
        auto text = zip_file_get_comment(handle_.get(), index, &length, flags);

        if (text == nullptr)
            throw std::runtime_error(zip_strerror(handle_.get()));

        return std::string(text, length);
    }

    /**
     * Set the archive comment.
     *
     * \param comment the comment
     * \throw std::runtime_error on errors
     */
    void set_comment(const std::string& comment)
    {
        if (zip_set_archive_comment(handle_.get(), comment.c_str(), comment.size()) < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));
    }

    /**
     * Get the archive comment.
     *
     * \param flags the optional flags
     * \return the comment
     * \throw std::runtime_error on errors
     */
    std::string comment(flags_t flags = 0) const
    {
        int length = 0;
        auto text = zip_get_archive_comment(handle_.get(), &length, flags);

        if (text == nullptr)
            throw std::runtime_error(zip_strerror(handle_.get()));

        return std::string(text, static_cast<std::size_t>(length));
    }

    /**
     * Check if a file exists on the archive.
     *
     * \param name the name
     * \param flags the optional flags
     * \return if the file exists
     */
    bool exists(const std::string& name, flags_t flags = 0) const noexcept
    {
        return zip_name_locate(handle_.get(), name.c_str(), flags) >= 0;
    }

    /**
     * Locate a file on the archive.
     *
     * \param name the name
     * \param flags the optional flags
     * \return the index
     * \throw std::runtime_error on errors
     */
    int64_t find(const std::string& name, flags_t flags = 0) const
    {
        auto index = zip_name_locate(handle_.get(), name.c_str(), flags);

        if (index < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));

        return index;
    }

    /**
     * Get information about a file.
     *
     * \param name the name
     * \param flags the optional flags
     * \return the structure
     * \throw std::runtime_error on errors
     */
    libzip::stat stat(const std::string& name, flags_t flags = 0) const
    {
        libzip::stat st;

        if (zip_stat(handle_.get(), name.c_str(), flags, &st) < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));

        return st;
    }

    /**
     * Get information about a file. Overloaded function.
     *
     * \param index the file index in the archive
     * \param flags the optional flags
     * \return the structure
     * \throw std::runtime_error on errors
     */
    libzip::stat stat(uint64_t index, flags_t flags = 0) const
    {
        libzip::stat st;

        if (zip_stat_index(handle_.get(), index, flags, &st) < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));

        return st;
    }

    /**
     * Add a file to the archive.
     *
     * \param source the source
     * \param name the name entry in the archive
     * \param flags the optional flags
     * \return the new index in the archive
     * \throw std::runtime_error on errors
     * \see source::file
     * \see source::buffer
     */
    int64_t add(const source& source, const std::string& name, flags_t flags = 0)
    {
        auto src = source(handle_.get());
        auto ret = zip_file_add(handle_.get(), name.c_str(), src, flags);

        if (ret < 0) {
            zip_source_free(src);
            throw std::runtime_error(zip_strerror(handle_.get()));
        }

        return ret;
    }

    /**
     * Create a directory in the archive.
     *
     * \param directory the directory name
     * \param flags the optional flags
     * \return the new index in the archive
     * \throw std::runtime_error on errors
     */
    int64_t mkdir(const std::string& directory, flags_t flags = 0)
    {
        auto ret = zip_dir_add(handle_.get(), directory.c_str(), flags);

        if (ret < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));

        return ret;
    }

    /**
     * Replace an existing file in the archive.
     *
     * \param source the source
     * \param index the file index in the archiev
     * \param flags the optional flags
     * \throw std::runtime_error on errors
     */
    void replace(const source& source, uint64_t index, flags_t flags = 0)
    {
        auto src = source(handle_.get());

        if (zip_file_replace(handle_.get(), index, src, flags) < 0) {
            zip_source_free(src);
            throw std::runtime_error(zip_strerror(handle_.get()));
        }
    }

    /**
     * Open a file in the archive.
     *
     * \param name the name
     * \param flags the optional flags
     * \param password the optional password
     * \return the opened file
     * \throw std::runtime_error on errors
     */
    file open(const std::string& name, flags_t flags = 0, const std::string& password = "")
    {
        struct zip_file* file;

        if (password.size() > 0)
            file = zip_fopen_encrypted(handle_.get(), name.c_str(), flags, password.c_str());
        else
            file = zip_fopen(handle_.get(), name.c_str(), flags);

        if (file == nullptr)
            throw std::runtime_error(zip_strerror(handle_.get()));

        return file;
    }

    /**
     * Open a file in the archive. Overloaded function.
     *
     * \param index the file index in the archive
     * \param flags the optional flags
     * \param password the optional password
     * \return the opened file
     * \throw std::runtime_error on errors
     */
    file open(uint64_t index, flags_t flags = 0, const std::string& password = "")
    {
        struct zip_file* file;

        if (password.size() > 0)
            file = zip_fopen_index_encrypted(handle_.get(), index, flags, password.c_str());
        else
            file = zip_fopen_index(handle_.get(), index, flags);

        if (file == nullptr)
            throw std::runtime_error(zip_strerror(handle_.get()));

        return file;
    }

    /**
     * Rename an existing entry in the archive.
     *
     * \param index the file index in the archive
     * \param name the new name
     * \param flags the optional flags
     * \throw std::runtime_error on errors
     */
    inline void rename(uint64_t index, const std::string& name, flags_t flags = 0)
    {
        if (zip_file_rename(handle_.get(), index, name.c_str(), flags) < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));
    }

    /**
     * Set file compression.
     *
     * \param index the file index in the archive
     * \param comp the compression
     * \param flags the optional flags
     * \throw std::runtime_error on errors
     */
    inline void set_file_compression(uint64_t index, int32_t comp, uint32_t flags = 0)
    {
        if (zip_set_file_compression(handle_.get(), index, comp, flags) < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));
    }

    /**
     * Delete a file from the archive.
     *
     * \param index the file index in the archive
     * \throw std::runtime_error on errors
     */
    inline void remove(uint64_t index)
    {
        if (zip_delete(handle_.get(), index) < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));
    }

    /**
     * Get the number of entries in the archive.
     *
     * \param flags the optional flags
     * \return the number of entries
     */
    inline int64_t num_entries(flags_t flags = 0) const noexcept
    {
        return zip_get_num_entries(handle_.get(), flags);
    }

    /**
     * Revert changes on the file.
     *
     * \param index the index
     * \throw std::runtime_error on errors
     */
    inline void unchange(uint64_t index)
    {
        if (zip_unchange(handle_.get(), index) < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));
    }

    /**
     * Revert all changes.
     *
     * \throw std::runtime_error on errors
     */
    inline void unchange_all()
    {
        if (zip_unchange_all(handle_.get()) < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));
    }

    /**
     * Revert changes to archive.
     *
     * \throw std::runtime_error on errors
     */
    void unchange_archive()
    {
        if (zip_unchange_archive(handle_.get()) < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));
    }

    /**
     * Set the defaut password.
     *
     * \param password the password or empty to unset it
     * \throw std::runtime_error on errors
     */
    void set_default_password(const std::string& password = "")
    {
        auto cstr = (password.size() > 0) ? password.c_str() : nullptr;

        if (zip_set_default_password(handle_.get(), cstr) < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));
    }

    /**
     * Set an archive flag.
     *
     * \param flag the flag to set
     * \param value the value
     * \throw std::runtime_error on errors
     */
    inline void set_flag(flags_t flag, int value)
    {
        if (zip_set_archive_flag(handle_.get(), flag, value) < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));
    }

    /**
     * Get an archive flag.
     *
     * \param which which flag
     * \param flags the optional flags
     * \return the value
     * \throw std::runtime_error on errors
     */
    inline int flag(flags_t which, flags_t flags = 0) const
    {
        auto ret = zip_get_archive_flag(handle_.get(), which, flags);

        if (ret < 0)
            throw std::runtime_error(zip_strerror(handle_.get()));

        return ret;
    }
};

} // !libzip

#endif // !ZIP_HPP
