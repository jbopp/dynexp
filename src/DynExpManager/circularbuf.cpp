// This file is part of DynExp.

#include "stdafx.h"
#include "circularbuf.h"

namespace Util
{
	circularbuf::circularbuf(size_t size)
	{
		buffer.resize(size);

		clear();
	}

	bool circularbuf::empty()
	{
		sync();

		return !avail_get_count();
	}

	size_t circularbuf::gsize()
	{
		sync();

		return avail_get_count();
	}

	size_t circularbuf::psize() const noexcept
	{
		return buffer.size();
	}

	circularbuf::pos_type circularbuf::gtellp() const noexcept
	{
		return gptr() - eback();
	}

	circularbuf::pos_type circularbuf::ptellp() const noexcept
	{
		return pptr() - pbase();
	}

	void circularbuf::clear()
	{
		setp(buffer.data(), buffer.data() + buffer.size());
		setg(buffer.data(), buffer.data(), buffer.data());

		put_overflowed = false;
	}

	void circularbuf::resize(size_t size)
	{
		buffer.resize(size);

		auto ppos = ptellp();
		setp(buffer.data(), buffer.data() + buffer.size());

		auto buffer_size = to_pos_type(buffer.size());
		if (buffer_size > std::numeric_limits<int>::max())
			throw std::overflow_error("Cannot convert a buffer size to int since this would cause an overflow in circularbuf::resize().");
		pbump(ppos >= buffer_size ? buffer_size : ppos);

		auto gpos = gtellp();
		auto gsz = gsize();
		setg(buffer.data(),
			gpos >= buffer_size ? buffer.data() + buffer_size : buffer.data() + gpos,
			gsz >= buffer.size() ? buffer.data() + buffer.size() : buffer.data() + gsz);
	}

	std::streamsize circularbuf::avail_get_count() const noexcept
	{
		return egptr() - eback();
	}

	circularbuf::int_type circularbuf::overflow(int_type c)
	{
		// Return eof if put buffer is empty or c is eof
		if (!psize() || c == traits_type::eof())
			return traits_type::eof();

		// Restart from beginning if at buffer's end
		if (pptr() == epptr())
			setp(pbase(), epptr());

		*pptr() = c;
		pbump(1);

		put_overflowed = true;
		sync();

		return int_type();
	}

	circularbuf::int_type circularbuf::underflow()
	{
		sync();

		// Return eof if read buffer is empty
		if (empty())
			return traits_type::eof();

		// Restart from beginning if at buffer's end
		if (gptr() == egptr())
			setg(eback(), eback(), egptr());

		return traits_type::to_int_type(*gptr());
	}

	std::streamsize circularbuf::showmanyc()
	{
		// in_avail() returns egptr() - gptr() and if this is 0, showmanyc() is returned.
		auto count = avail_get_count();

		return count ? count : -1;
	}

	circularbuf::int_type circularbuf::pbackfail(int_type c)
	{
		sync();

		// Return eof if buffer is empty
		if (empty())
			return traits_type::eof();

		// At very beginning? Set to end, otherwise step back.
		if (gptr() == eback())
			setg(eback(), egptr() - 1, egptr());
		else
			gbump(-1);
		
		// Replace character?
		if (c != traits_type::eof())
			*gptr() = c;

		return traits_type::to_int_type(*gptr());
	}

	int circularbuf::sync()
	{
		if (egptr() < (put_overflowed ? epptr() : pptr()))
			setg(eback(), gptr(), put_overflowed ? epptr() : pptr());

		put_overflowed = false;

		return 0;
	}

	circularbuf::pos_type circularbuf::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which)
	{
		bool in = which & std::ios_base::in;
		bool out = which & std::ios_base::out;
		pos_type new_pos(pos_type(off_type(-1)));

		if (in)
		{
			pos_type abs_pos_in = (dir == std::ios_base::end ? pos_type(avail_get_count()) :
				(dir == std::ios_base::cur ? gtellp() : pos_type(0)));
			if (!off)
				return abs_pos_in;
			if (avail_get_count())
				abs_pos_in += std::abs(off % avail_get_count()) * (off >= 0 ? 1 : -1);
			else
				abs_pos_in += off;

			// Check boundaries
			if (abs_pos_in < 0 || avail_get_count() <= abs_pos_in)
				abs_pos_in += avail_get_count() * (abs_pos_in < 0 ? 1 : -1);

			new_pos = seekpos(abs_pos_in, std::ios_base::in);
		}

		// Only do something if there wasn't any error.
		if (out && ((in && new_pos >= 0) || !in))
		{
			pos_type abs_pos_out = (dir == std::ios_base::end ? pos_type(buffer.size()) :
				(dir == std::ios_base::cur ? ptellp() : pos_type(0)));
			if (!off)
				return abs_pos_out;
			if (buffer.size())
				abs_pos_out += std::abs(off % to_pos_type(buffer.size())) * (off >= 0 ? 1 : -1);
			else
				abs_pos_out += off;

			// Check boundaries
			if (abs_pos_out < 0 || to_pos_type(buffer.size()) <= abs_pos_out)
				abs_pos_out += to_pos_type(buffer.size()) * (abs_pos_out < 0 ? 1 : -1);

			auto new_pos_out = seekpos(abs_pos_out, std::ios_base::out);
			if (new_pos_out < 0 || !in)
				new_pos = new_pos_out;
		}		

		if (in && out)
			return pos_type(off_type(-1));
		else
			return new_pos;
	}

	circularbuf::pos_type circularbuf::seekpos(pos_type pos, std::ios_base::openmode which)
	{
		bool fail = false;

		if (which & std::ios_base::in)
		{
			if (avail_get_count() <= pos || pos < 0)
				fail = true;
			else
				setg(eback(), eback() + pos, egptr());
		}

		if (which & std::ios_base::out && !fail)
		{
			if (to_pos_type(buffer.size()) <= pos || pos < 0)
				fail = true;
			else
			{
				setp(pbase(), epptr());
				pbump(pos);
			}
		}

		return fail ? pos_type(off_type(-1)) : pos;
	}
}