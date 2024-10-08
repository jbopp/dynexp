/*
$Id: PythonSyntaxHighlighter.cpp 167 2013-11-03 17:01:22Z oliver $
This is a C++ port of the following PyQt example
http://diotavelli.net/PyQtWiki/Python%20syntax%20highlighting
C++ port by Frankie Simon (www.kickdrive.de, www.fuh-edv.de)

The following free software license applies for this file ("X11 license"):

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "moc_PythonSyntaxHighlighter.cpp"
#include "PythonSyntaxHighlighter.h"

void PythonSyntaxHighlighter::Init()
{
	keywords = QStringList() << "and" << "assert" << "break" << "class" << "continue" << "def" <<
		"del" << "elif" << "else" << "except" << "exec" << "finally" <<
		"for" << "from" << "global" << "if" << "import" << "in" <<
		"is" << "lambda" << "not" << "or" << "pass" << "print" <<
		"raise" << "return" << "try" << "while" << "yield" <<
		"None" << "True" << "False";

	operators = QStringList() << "=" <<
		// Comparison
		"==" << "!=" << "<" << "<=" << ">" << ">=" <<
		// Arithmetic
		"\\+" << "-" << "\\*" << "/" << "//" << "%" << "\\*\\*" <<
		// In-place
		"\\+=" << "-=" << "\\*=" << "/=" << "%=" <<
		// Bitwise
		"\\^" << "\\|" << "&" << "~" << ">>" << "<<";

	braces = QStringList() << "{" << "}" << "\\(" << "\\)" << "\\[" << "]";

	basicStyles.insert("keyword", getTextCharFormat("blue"));
	basicStyles.insert("operator", getTextCharFormat("red"));
	basicStyles.insert("brace", getTextCharFormat("darkGray"));
	basicStyles.insert("defclass", getTextCharFormat("black", "bold"));
	basicStyles.insert("brace", getTextCharFormat("darkGray"));
	basicStyles.insert("string", getTextCharFormat("magenta"));
	basicStyles.insert("string2", getTextCharFormat("darkMagenta"));
	basicStyles.insert("comment", getTextCharFormat("darkGreen", "italic"));
	basicStyles.insert("self", getTextCharFormat("black", "italic"));
	basicStyles.insert("numbers", getTextCharFormat("brown"));

	triSingleQuote.setPattern("'''");
	triDoubleQuote.setPattern("\"\"\"");

	initializeRules();
}

void PythonSyntaxHighlighter::initializeRules()
{
	foreach(QString currKeyword, keywords)
	{
		rules.append(HighlightingRule(QString("\\b%1\\b").arg(currKeyword), 0, basicStyles.value("keyword")));
	}
	foreach(QString currOperator, operators)
	{
		rules.append(HighlightingRule(QString("%1").arg(currOperator), 0, basicStyles.value("operator")));
	}
	foreach(QString currBrace, braces)
	{
		rules.append(HighlightingRule(QString("%1").arg(currBrace), 0, basicStyles.value("brace")));
	}
	// 'self'
	rules.append(HighlightingRule("\\bself\\b", 0, basicStyles.value("self")));

	// Double-quoted string, possibly containing escape sequences
 // FF: originally in python : r'"[^"\\]*(\\.[^"\\]*)*"'
	rules.append(HighlightingRule("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"", 0, basicStyles.value("string")));
	// Single-quoted string, possibly containing escape sequences
 // FF: originally in python : r"'[^'\\]*(\\.[^'\\]*)*'"
	rules.append(HighlightingRule("'[^'\\\\]*(\\\\.[^'\\\\]*)*'", 0, basicStyles.value("string")));

	// 'def' followed by an identifier
 // FF: originally: r'\bdef\b\s*(\w+)'
	rules.append(HighlightingRule("\\bdef\\b\\s*(\\w+)", 1, basicStyles.value("defclass")));
	//  'class' followed by an identifier
 // FF: originally: r'\bclass\b\s*(\w+)'
	rules.append(HighlightingRule("\\bclass\\b\\s*(\\w+)", 1, basicStyles.value("defclass")));

	// From '#' until a newline
 // FF: originally: r'#[^\\n]*'
	rules.append(HighlightingRule("#[^\\n]*", 0, basicStyles.value("comment")));

	// Numeric literals
	rules.append(HighlightingRule("\\b[+-]?[0-9]+[lL]?\\b", 0, basicStyles.value("numbers"))); // r'\b[+-]?[0-9]+[lL]?\b'
	rules.append(HighlightingRule("\\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\\b", 0, basicStyles.value("numbers"))); // r'\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\b'
	rules.append(HighlightingRule("\\b[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\\b", 0, basicStyles.value("numbers"))); // r'\b[+-]?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\b'
}

void PythonSyntaxHighlighter::highlightBlock(const QString& text)
{
	foreach(HighlightingRule currRule, rules)
	{
		auto matches = currRule.pattern.globalMatch(text);
		while (matches.hasNext())
		{
			// Get index of Nth match
			auto match = matches.next();
			auto idx = match.capturedStart(currRule.nth);
			int length = match.captured(currRule.nth).length();
			setFormat(idx, length, currRule.format);
		}
	}

	setCurrentBlockState(0);

	// Do multi-line strings
	bool isInMultilne = matchMultiline(text, triSingleQuote, 1, basicStyles.value("string2"));
	if (!isInMultilne)
		isInMultilne = matchMultiline(text, triDoubleQuote, 2, basicStyles.value("string2"));
}

bool PythonSyntaxHighlighter::matchMultiline(const QString& text, const QRegularExpression& delimiter, const int inState, const QTextCharFormat& style)
{
	int start = -1;
	int add = -1;
	int end = -1;
	int length = 0;

	// If inside triple-single quotes, start at 0
	if (previousBlockState() == inState) {
		start = 0;
		add = 0;
	}
	// Otherwise, look for the delimiter on this line
	else {
		auto match = delimiter.match(text);
		start = match.capturedStart();
		// Move past this match
		add = match.capturedLength();
	}

	// As long as there's a delimiter match on this line...
	while (start >= 0) {
		// Look for the ending delimiter
		auto match = delimiter.match(text, static_cast<qsizetype>(start) + add);
		end = match.capturedStart();
		// Ending delimiter on this line?
		if (end >= add) {
			length = static_cast<qsizetype>(end) - start + add + match.capturedLength();
			setCurrentBlockState(0);
		}
		// No; multi-line string
		else {
			setCurrentBlockState(inState);
			length = text.length() - start + add;
		}
		// Apply formatting and look for next
		setFormat(start, length, style);
		match = delimiter.match(text, static_cast<qsizetype>(start) + length);
		start = match.capturedStart();
	}
	// Return True if still inside a multi-line string, False otherwise
	if (currentBlockState() == inState)
		return true;
	else
		return false;
}

const QTextCharFormat PythonSyntaxHighlighter::getTextCharFormat(const QString& colorName, const QString& style)
{
	QTextCharFormat charFormat;
	QColor color(colorName);
	charFormat.setForeground(color);
	if (style.contains("bold", Qt::CaseInsensitive))
		charFormat.setFontWeight(QFont::Bold);
	if (style.contains("italic", Qt::CaseInsensitive))
		charFormat.setFontItalic(true);
	return charFormat;
}