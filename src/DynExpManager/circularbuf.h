// This file is part of DynExp.

/**
 * @file circularbuf.h
 * @brief Implements a circular stream buffer derived from @p std::streambuf to be used with
 * standard library streams like @p std::iostream.
*/

#pragma once

#include <streambuf>

namespace Util
{
	/**
	 * @brief Circular stream buffer to be used with the standard library's stream classes.
	 * Reading from or writing behind the buffer's end continues at the buffer's beginning.
	*/
	class circularbuf : public std::streambuf
	{
	public:
		/**
		 * @brief Data type of a single character stored in the buffer.
		*/
		using CharT = char;

		/**
		 * @brief Constructs a new @p circularbuf instance with a #buffer of size @p size.
		 * The put area will span the entire #buffer, the get area will be empty.
		 * @param size Initial buffer size
		*/
		circularbuf(size_t size);

		/**
		 * @brief Indicates whether characters can be read from the get area.
		 * Not const since @p sync() needs to be called to get correct size after a character has been written recently.
		 * @return Returns true if @p avail_get_count() returns 0, false otherwise.
		*/
		bool empty();
		
		/**
		 * @brief Returns the size of the get area.
		 * Not const since @p sync() needs to be called to get correct size after a character has been written recently. 
		 * @return Get area size
		*/
		size_t gsize();

		/**
		 * @brief Returns the size of the put area.
		 * @return Put area size
		*/
		size_t psize() const noexcept;

		/**
		 * @brief Indicates the get pointer's position within the get area
		 * @return Returns the distance between the get area's beginning and the get pointer's position.
		*/
		pos_type gtellp() const noexcept;

		/**
		 * @brief Indicates the put pointer's position within the put area
		 * @return Returns the distance between the put area's beginning and the put pointer's position.
		*/
		pos_type ptellp() const noexcept;

		/**
		 * @brief Resets the put areas to the entire area of #buffer. The get area will be
		 * empty. The get and put pointers will point to the beginning of their respective
		 * areas. Resets #put_overflowed.
		*/
		void clear();

		/**
		 * @brief Resizes the size of #buffer to @p size.
		 * The put area will span the entire #buffer. If the buffer is enlarged, the get area will
		 * maintain its size. If the buffer is shrunk, the get area will span the entire #buffer.
		 * If the original put (get) pointer falls behind the new put (get) area's length, it is set
		 * to the put (get) area's end, otherwise it will maintain its old position. 
		 * @param size New buffer size to apply
		 * @throws std::overflow_error is thrown if @p size does not fit into @p int.
		*/
		void resize(size_t size);

	private:
		/**
		 * @brief Returns the amount of characters available to be read from the current get pointer
		 * position to the get area's end.
		 * @return Amount of available characters
		*/
		std::streamsize avail_get_count() const noexcept;

		/** @name Overridden
		 * Overridden from standard library. Refer to documentation of @p std::streambuf.
		*/
		///@{
		virtual int_type overflow(int_type c = traits_type::eof()) override;	//!< Refer to documentation of @p std::streambuf.
		virtual int_type underflow() override;									//!< Refer to documentation of @p std::streambuf.
		virtual std::streamsize showmanyc() override;							//!< Refer to documentation of @p std::streambuf.
		virtual int_type pbackfail(int_type c = traits_type::eof()) override;	//!< Refer to documentation of @p std::streambuf.

		/**
		 * @brief Expands the get area to the size of the put area. Resets #put_overflowed.
		 * @return Returns 0.
		*/
		virtual int sync() override;
		///@}

		/**
		 * @brief Sets the position of the pointer(s) specified by @p which to the position @p pos relative
		 * to @p dir. Takes the stream's circularity into account.
		 * @param off Destiny position (relative to @p dir).
		 * @param dir Set position relative to stream's beginning (@p std::ios_base::beg), its end
		 * (@p std::ios_base::end) or the current position (@p std::ios_base::cur).
		 * @param which Combination of flags @p std::ios_base::in and @p std::ios_base::out specifying
		 * get or put pointers.
		 * @return New position @p pos in case of success, -1 otherwise. If the get and the put pointers
		 * are both successfully moved, the new position of the get pointer is returned.
		*/
		virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir,
			std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;

		/**
		 * @brief Sets the position of the pointer(s) specified by @p which to the absolute position @p pos.
		 * @param pos Destiny position (absolute).
		 * @param which Combination of flags @p std::ios_base::in and @p std::ios_base::out specifying get
		 * or put pointers.
		 * @return New position @p pos in case of success, -1 otherwise.
		*/
		virtual pos_type seekpos(pos_type pos,
			std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;

		/**
		 * @brief Converts an unsigned, integral number type to @p pos_type.
		 * @tparam T Type to convert from
		 * @param value Value to convert
		 * @return Returns the converted value.
		 * @throws std::overflow_error is thrown if @p value is larger than the maximal number storable
		 * in @p pos_type.
		*/
		template <typename T, std::enable_if_t<
			std::is_unsigned_v<T>, int> = 0
		>
		inline pos_type to_pos_type(const T value)
		{
			if (value > static_cast<T>(std::numeric_limits<std::make_signed_t<T>>::max()))
				throw std::overflow_error("Cannot convert a value into pos_type since this would cause an overflow in circularbuf::to_pos_type().");

			return static_cast<std::make_signed_t<T>>(value);
		}

		/**
		 * @brief The buffer itself.
		*/
		std::vector<CharT> buffer;

		/**
		 * @brief Indicates wheter an overflow occurred writing to the buffer. If this was the case,
		 * subsequent calls to @p sync() have to enlarge the get area to the put area's size
		 * regardless of the current put pointer's position.
		*/
		bool put_overflowed;
	};
}