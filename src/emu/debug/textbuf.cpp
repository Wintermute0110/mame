// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    textbuf.c

    Debugger text buffering engine.

***************************************************************************/

#include "textbuf.h"

#include <new>



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_LINE_LENGTH         250



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct text_buffer
{
	text_buffer(u32 bytes, u32 lines) noexcept
		: buffer(new (std::nothrow) char [bytes])
		, lineoffs(new (std::nothrow) s32 [lines])
		, bufsize(buffer ? bytes : 0)
		, linesize(lineoffs ? lines : 0)
	{
	}
	~text_buffer()
	{
		if (buffer)
			delete [] buffer;
		if (lineoffs)
			delete [] lineoffs;
	}

	char *const buffer;
	s32 *const  lineoffs;
	s32 const   bufsize;
	s32         bufstart = 0;
	s32         bufend = 0;
	s32 const   linesize;
	s32         linestart = 0;
	s32         lineend = 0;
	u32         linestartseq = 0;
	s32         maxwidth = 0;

	/*-------------------------------------------------
	    buffer_used - return the number of bytes
	    currently held in the buffer
	-------------------------------------------------*/

	s32 buffer_used() const noexcept
	{
		s32 const used(bufend - bufstart);
		return (used < 0) ? (used + bufsize) : used;
	}

	/*-------------------------------------------------
	    buffer_space - return the number of bytes
	    available in the buffer
	-------------------------------------------------*/
	
	s32 buffer_space() const noexcept
	{
		return bufsize - buffer_used();
	}
};



/***************************************************************************

    Buffer object management

***************************************************************************/

/*-------------------------------------------------
    text_buffer_alloc - allocate a new text buffer
-------------------------------------------------*/

text_buffer *text_buffer_alloc(u32 bytes, u32 lines)
{
	// allocate memory for the text buffer object
	text_buffer *const text(new (std::nothrow) text_buffer(bytes, lines));

	if (!text)
		return nullptr;

	if (!text->buffer || !text->lineoffs)
	{
		delete text;
		return nullptr;
	}

	// initialize the buffer description
	text_buffer_clear(text);

	return text;
}


/*-------------------------------------------------
    text_buffer_free - free a previously allocated
    text buffer
-------------------------------------------------*/

void text_buffer_free(text_buffer *text)
{
	delete text;
}


/*-------------------------------------------------
    text_buffer_clear - clear a text buffer
-------------------------------------------------*/

void text_buffer_clear(text_buffer *text)
{
	/* reset all the buffer pointers and other bits */
	text->bufstart = 0;
	text->bufend = 0;

	text->linestart = 0;
	text->lineend = 0;
	text->linestartseq = 0;

	text->maxwidth = 0;

	/* create the initial line */
	text->lineoffs[0] = 0;
	text->buffer[text->lineoffs[0]] = 0;
}



/***************************************************************************

    Adding data to the buffer

***************************************************************************/

/*-------------------------------------------------
    text_buffer_print - print data to the text
    buffer
-------------------------------------------------*/

void text_buffer_print(text_buffer *text, const char *data)
{
	text_buffer_print_wrap(text, data, 10000);
}


/*-------------------------------------------------
    text_buffer_print_wrap - print data to the
    text buffer with word wrapping to a given
    column
-------------------------------------------------*/

void text_buffer_print_wrap(text_buffer *text, const char *data, int wrapcol)
{
	s32 stopcol = (wrapcol < MAX_LINE_LENGTH) ? wrapcol : MAX_LINE_LENGTH;
	s32 needed_space;

	/* we need to ensure there is enough space for this string plus enough for the max line length */
	needed_space = s32(strlen(data)) + MAX_LINE_LENGTH;

	/* make space in the buffer if we need to */
	while (text->buffer_space() < needed_space && text->linestart != text->lineend)
	{
		text->linestartseq++;
		if (++text->linestart >= text->linesize)
			text->linestart = 0;
		text->bufstart = text->lineoffs[text->linestart];
	}

	/* now add the data */
	for ( ; *data; data++)
	{
		int ch = *data;
		int linelen;

		/* a CR resets our position */
		if (ch == '\r')
			text->bufend = text->lineoffs[text->lineend];

		/* non-CR data is just characters */
		else if (ch != '\n')
			text->buffer[text->bufend++] = ch;

		/* an explicit newline or line-too-long condition inserts a newline */
		linelen = text->bufend - text->lineoffs[text->lineend];
		if (ch == '\n' || linelen >= stopcol)
		{
			int overflow = 0;

			/* if we're wrapping, back off until we hit a space */
			if (linelen >= wrapcol)
			{
				/* scan backwards, removing characters along the way */
				overflow = 1;
				while (overflow < linelen && text->buffer[text->bufend - overflow] != ' ')
					overflow++;

				/* if we found a space, take it; otherwise, reset and pretend we didn't try */
				if (overflow < linelen)
					linelen -= overflow;
				else
					overflow = 0;
			}

			/* did we beat the max width */
			if (linelen > text->maxwidth)
				text->maxwidth = linelen;

			/* append a terminator */
			if (overflow == 0)
				text->buffer[text->bufend++] = 0;
			else
				text->buffer[text->bufend - overflow] = 0;

			/* determine what the next line will be */
			if (++text->lineend >= text->linesize)
				text->lineend = 0;

			/* if we're out of lines, consume the next one */
			if (text->lineend == text->linestart)
			{
				text->linestartseq++;
				if (++text->linestart >= text->linesize)
					text->linestart = 0;
				text->bufstart = text->lineoffs[text->linestart];
			}

			/* if we don't have enough room in the buffer for a max line, wrap to the start */
			if (text->bufend + MAX_LINE_LENGTH + 1 >= text->bufsize)
				text->bufend = 0;

			/* create a new empty line */
			text->lineoffs[text->lineend] = text->bufend - (overflow ? (overflow - 1) : 0);
		}
	}

	/* nullptr terminate what we have on this line */
	text->buffer[text->bufend] = 0;
}



/***************************************************************************

    Reading data from the buffer

***************************************************************************/

/*-------------------------------------------------
    text_buffer_max_width - return the maximum
    width of all lines seen so far
-------------------------------------------------*/

u32 text_buffer_max_width(text_buffer *text)
{
	return text->maxwidth;
}


/*-------------------------------------------------
    text_buffer_num_lines - return the number of
    lines in the text buffer
-------------------------------------------------*/

u32 text_buffer_num_lines(text_buffer *text)
{
	s32 lines = text->lineend + 1 - text->linestart;
	if (lines <= 0)
		lines += text->linesize;
	return lines;
}


/*-------------------------------------------------
    text_buffer_line_index_to_seqnum - convert a
    line index into a sequence number
-------------------------------------------------*/

u32 text_buffer_line_index_to_seqnum(text_buffer *text, u32 index)
{
	return text->linestartseq + index;
}


/*-------------------------------------------------
    text_buffer_get_seqnum_line - get a pointer to
    an indexed line in the buffer
-------------------------------------------------*/

const char *text_buffer_get_seqnum_line(text_buffer *text, u32 seqnum)
{
	u32 numlines = text_buffer_num_lines(text);
	u32 index = seqnum - text->linestartseq;
	if (index >= numlines)
		return nullptr;
	return &text->buffer[text->lineoffs[(text->linestart + index) % text->linesize]];
}
