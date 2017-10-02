# module pybis.py
#
# Copyright (C) 2012 Russ Dill <Russ.Dill@asu.edu>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

import sys
import math
import copy
import re
from pyparsing import alphanums, alphas, CaselessKeyword, Dict, Each, Forward, Group, LineStart, LineEnd, Literal, NotAny, oneOf, Optional, ParseException, ParserElement, ParseResults, printables, restOfLine, Token, Suppress, Word
from collections import OrderedDict
from numpy import zeros

__all__ = ["Range", "IBSParser", "PKGParser", "EBDParser", "dump"]

def ParseReal(val):
    """Parse an IBIS formatted number."""
    try:
        return float(val)
    except:
        ext = val.lstrip('+-0123456789.eE')
        e = 1
        if len(ext):
            val = val[0:-len(ext)]
            e = 'fpnum.kMGT'.find(ext[0])
            if e == -1:
                e = 5
            e = 10**((e - 5) * 3)
        # 'e' or 'E' can be a unit
        if val[-1] == 'e' or val[-1] == 'E':
            val = val[:-1]
        return float(val) * e

def in_range(lower, upper):
    """Throw an exception if a number is not in a range."""
    def func(n):
        if not lower <= n <= upper:
            raise Exception("'{}' is not in range '{}'".format(n, (lower, upper)))
        return n
    return func

positive = in_range(0, float("inf"))

class Real(Token):
    """pyparse a Real."""
    def __init__(self, check=lambda f: f):
        super(Real, self).__init__()
        self.check = check
        self.name = "Real"

    def parseImpl(self, instring, loc, doActions=True):
        tokens = re.split("[^a-zA-Z0-9\.\-\+]+", instring[loc:], 1)
        try:
            return loc + len(tokens[0]), self.check(ParseReal(tokens[0]))
        except:
            raise ParseException(tokens[0], loc, "Could not parse float, '{}'".format(tokens[0]))

class Integer(Token):
    """pyparse an Integer."""
    def __init__(self, check=lambda f: f):
        super(Integer, self).__init__()
        self.check = check

    def parseImpl(self, instring, loc, doActions=True):
        tokens = re.split("[^0-9\-\+]+", instring[loc:], 1)
        val = tokens[0]
        try:
            return loc + len(val), self.check(int(val))
        except:
            raise ParseException(tokens[0], loc, "Could not parse integer")

class NAReal(Real):
    """pyparse an "optional" number, gives 'None' for 'NA'."""
    def parseImpl(self, instring, loc, doActions=True):
        if instring[loc:2+loc] == "NA":
            return loc + 2, None
        return super(NAReal, self).parseImpl(instring, loc, doActions)

class Range(list):
    """A typ, min, max range object.
       [0-2] -                  Return raw values (0 = typ, 1 = min, 2 = max).
       (0-2) -                  Return typ for min or max if they are 'None'.
       (0-2, invert=False) -    Ensure max > min.
       (0-2, invert=True) -     Ensure mix > max.
       .typ, .min, .max -       Easy accessors for (0-2).
       .norm -                  Return version of object where max > min.
       .inv -                   return version of object where min > max.
    """
    def __getattr__(self, name):
        if name == "typ":
            return self[0]
        elif name == "min":
            return self(1)
        elif name == "max":
            return self(2)
        elif name == "norm":
            if self.min > self.max:
                return Range([self[0], self[2], self[1]])
            else:
                return self
        elif name == "inv":
            if self.min < self.max:
                return Range([self[0], self[2], self[1]])
            else:
                return self
        else:
            raise AttributeError(name)

    def __call__(self, n, invert=None):
        if n == 0:
            return self[0]
        elif n < 3:
            if invert is not None and invert == self.min < self.max:
                n = 3 - n
            return self[n] if self[n] is not None else self[0]
        else:
            raise Exception

class RealRange(Token):
    """pyparser for a line of 'count' Real tokens.
       'check' should raise Exception on invalid value.
    """
    def __init__(self, count=3, check=lambda f: f):
        self.count = count * 2 - 1
        self.check = check
        super(RealRange, self).__init__()

    def parseImpl(self, instring, loc, doActions=True):
        tokens = re.split("([^a-zA-Z0-9\.\-\+]+)", instring[loc:])
        ret = Range()
        intok = True
        for tok in tokens[:self.count]:
            if intok:
                if len(ret) and tok == "NA":
                    ret.append(None)
                else:
                    try:
                        ret.append(self.check(ParseReal(tok)))
                    except:
                        raise ParseException(tok, loc, "Count not parse float")
            intok = not intok
            loc += len(tok)
        return loc, [ret]

def RampRange():
    """pyparser for range of IBIS ramp values (See Section 6 - [Ramp])."""
    ramp_data = Group(Real(check=positive) + Suppress(Literal("/")) + Real(check=positive))
    ret = (ramp_data -
        (ramp_data | CaselessKeyword("NA")) -
        (ramp_data | CaselessKeyword("NA"))
    )
    def fin(tokens):
        for i, t in enumerate(tokens[1:]):
            if t == "NA": tokens[i + 1] = None
        return Range(tokens.asList())
    ret.setParseAction(fin)
    return ret

def oneof(val):
    """pyparser for set of caseless keywords.
       All parsed values will be lowercase.
    """
    return oneOf(val.lower(), caseless=True)

def orderedDict(tokenlist):
    """pyparse action to create and OrderedDict from a set of tokens."""
    ret = OrderedDict()
    for i, tok in enumerate(tokenlist):
        ret[tok[0]] = tok[1]
    return ret

class IBISNode(OrderedDict):
    """IBIS parse results object.
       This object typically represents a section with named children.
       Because most IBIS keywords are case insensitive and many even don't
       distinguish between and '_' and ' ', this class allows loose access to
       children. All children are stored by their 'pretty_name', such as
       "Number of Sections". When an access is made, it is translated to a
       'simple_name' by lower() and translating ' ' and '/' to '_', '+' to 'p',
       and '-' to 'n'. The pretty_name is then looked up in an internal dict.
       The pretty_name is then used to find the child.
       In addition to normal dict() accessors, '__getattribute__' can also be
       used. For example, 'obj["Vinl+"] and obj.vinlp both work.
    """
    ibis_trans = str.maketrans('+- /', 'pn__')

    def __init__(self, *args, **kwds):
        object.__setattr__(self, 'pretty_names', dict())
        super(IBISNode, self).__init__(*args, **kwds)

    @staticmethod
    def simple_name(name):
        return name.translate(IBISNode.ibis_trans).lower()

    def pretty_name(self, name):
        return self.pretty_names[IBISNode.simple_name(name)]

    def __getattribute__(self, name):
        try:
            return OrderedDict.__getattribute__(self, name)
        except AttributeError:
            try:
                return self[name]
            except KeyError:
                raise AttributeError(name)

    def __setattr__(self, name, value):
        try:
            pretty = self.pretty_name(name)
            OrderedDict.__setitem__(self, pretty, value)
        except KeyError:
            OrderedDict.__setattr__(self, name, value)

    def __getitem__(self, name):
        return OrderedDict.__getitem__(self, self.pretty_name(name))

    def __setitem__(self, name, value):
        simple = IBISNode.simple_name(name)
        doset = False
        if simple not in self.pretty_names:
            pretty = name
            doset = True
        else:
            pretty = self.pretty_names[simple]
        OrderedDict.__setitem__(self, pretty, value)
        if doset:
            self.pretty_names[simple] = pretty

    def __delitem__(self, name):
        simple = IBISNode.simple_name(name)
        pretty = self.pretty_names.pop(simple)
        OrderedDict.__delitem__(self, pretty)

    def __contains__(self, name):
        return IBISNode.simple_name(name) in self.pretty_names

class container(object):
    def __init__(self):
        self.data = None

class Node(container):
    """Intermediate parse results holder."""
    def __init__(self):
        super(Node, self).__init__()
        self.children = IBISNode()
        self.parser = None
        self.parent = None
        self.data = None

    def __str__(self):
        return str(self.data)
    __repr__ = __str__

    def add(self, child):
        orig = container()
        if child.parser.key in self.children:
            orig.data = self.children[child.parser.key]
        else:
            # FIXME: Double assign of initial?
            orig.data = copy.deepcopy(child.parser.initvalue)
        child.parser.merge(orig, child)
        self.children[child.parser.key] = orig.data

    def __iadd__(self, child):
        self.add(child)
        return self

class Parse(object):
    """Base pybis Parse object."""
    def __init__(self, key, pyparser=None, default=None, initvalue=None, data_name=None, list_merge=False, asList=False, asDict=False, required=False):
        """key:         Name of element.
           pyparser:    Parser to call with pyparse
           default:     Default value of object if not found
           initvalue:   Default value of object on first merge
           data_name:   Make the data of this node a child with name 'data_name'
           list_merge:  Merge multiple copies together as list
           asList:      Interpret pyparse results as a list
           asDict:      Interpret pyparse results as a dict
           required:    raise Exception if not found
        """
        self.key = key
        self.flat_key = key.replace(' ', '_').lower()
        self.data_name = data_name
        self.default = default
        self.initvalue = initvalue
        self.pyparser = pyparser
        self.list_merge = list_merge
        if list_merge and initvalue is None:
            self.initvalue = list()
        self.asList = asList
        self.asDict = asDict
        self.children = OrderedDict()
        self.parent = None
        self.globals = None
        self.required = required

    def add(self, obj):
        obj.parent = self
        self.children[obj.key] = obj

    def __iadd__(self, obj):
        self.add(obj)
        return self

    def get_globals(self):
        """Get the global parse object, for things that are parseable in all contexts."""
        if self.globals is None and self.parent is not None:
            self.globals = self.parent.get_globals()
        return self.globals

    def find_parser(self, text):
        """Find a child parser that can parse 'text'."""
        for name, child in self.children.items():
            if child.can_parse(text):
                return child
        if self.get_globals() is not None:
            return self.globals.find_parser(text)
        return None

    def can_parse(self, text):
        """True if we can parse 'text'."""
        return False

    def initial(self, text, comment):
        """Parse the first line of text and return a Node object."""
        node = Node()
        node.parser = self
        # Fill in defaults
        if self.initvalue is not None and self.default is None:
            # FIXME: Maybe we can shortcut merge here...
            node.data = copy.deepcopy(self.initvalue)
        for key, parser in self.children.items():
            if parser.default is not None:
                node.children[key] = copy.deepcopy(parser.default)
        return node

    def parse(self, node, text, comment):
        """Parse a subsequent line of text, False means we can't."""
        return not text

    def pyparse(self, text):
        """Use self.pyparser to parse 'text', returns parse tokens.
           Returns 'text' if there is no pyparser object.
        """
        if self.pyparser is not None:
            try:
                return self.pyparser.parseString(text, parseAll=True)
            except ParseException as e:
                raise Exception("Failed to parse '{}': {}".format(text, e.msg))
        else:
            return text.strip()

    def fin(self, node):
        """Add a node to the parent."""
        if node.data is not None:
            self.flatten(node)
            if self.data_name: 
                node.children[self.data_name] = node.data

        for name, p in self.children.items():
            if p.required and not name in node.children:
                raise Exception("'{}' is missing required '{}'".format(self.key, name))

        if node.children:
            node.data = node.children

        # Oi, so some vendors think it'd be funny to add sections, such as
        # [Series Pin Mapping] without any data
        if node.data is not None and node.parent:
             node.parent += node

    def pop(self, new, name):
        """Remove 'name' from ParseResults 'new'."""
        ret = new.data[name]
        del new.data[name]
        if len(list(new.data.keys())) == 0:
            for i, item in enumerate(new.data.asList()):
                if item == ret:
                    del new.data[i]
                    break
        return ret

    def flatten(self, new):
        """Reformat pyparse results as what we'd expect."""
        if isinstance(new.data, ParseResults):
            if self.asList:
                new.data = new.data.asList()
            elif self.asDict:
                new.data = IBISNode(new.data.asDict())
            else:
                new.data = new.data[0]

    def merge(self, orig, new):
        """Merge two instances of the same parser under one parent."""
        if self.list_merge:
            # FIXME: Maybe specify this behavior?
            if isinstance(new.data, list):
                orig.data.extend(new.data)
            else:
                orig.data.append(new.data)
        elif orig.data != self.initvalue and orig.data != self.default:
            raise Exception("'{}' already assigned".format(self.key))
        else:
            orig.data = new.data

class Bracket(Parse):
    """An item that starts with '[<name>] ...'"""
    def can_parse(self, text):
        # FIXME: can_parse seems like a wasteful linear serach
        if text and text[0] == '[':
            sectionName, _, sectionText = text[1:].partition(']')
            sectionName = sectionName.replace(' ', '_').lower()
            if sectionName == self.flat_key:
                return True
        return False

    def initial(self, text, comment):
        node = super(Bracket, self).initial(text, comment)
        sectionName, _, sectionText = text[1:].partition(']')
        node.sectionText = sectionText.lstrip()
        return node

class Comment(Bracket):
    """IBIS '[Comment Char]' keyword."""
    def __init__(self, comment_holder):
        super(Comment, self).__init__("Comment Char")
        self.holder = comment_holder

    def initial(self, text, comment):
        node = super(Comment, self).initial(text, comment)

        if len(node.sectionText) == 0:
            # sectionText probably got commented out, no comment char change.
            pass
        elif not node.sectionText[1:].startsWith("_char"):
            raise Exception("Invalid format, expected '<char>_char'")
        elif not node.sectionText[0] in "!\"#$%&'()*,:;<>?@\\^`{|}~":
            raise Exception("Invalid comment char, '{}'".format(node.sectionText[0]))
        else:
            self.holder[0] = node.sectionText[0]
        return node

    def fin(self, node):
        """Ignore this node."""
        pass

class End(Bracket):
    """IBIS '[End]' keyword."""
    def initial(self, text, comment):
        """Return None as node, special meaning to close out parent."""
        return None

class Keyword(Bracket):
    """[<keyword>] <data>."""
    def initial(self, text, comment):
        node = super(Keyword, self).initial(text, comment)
        if not node.sectionText:
            raise Exception("Expected text after '{}'".format(text))
        node.data = self.pyparse(node.sectionText)
        return node

class Text(Bracket):
    """IBIS text section, such as '[Copyright]'
       Since some IBIS files include important copyright data in comments
       below the keyword, we capture all text, including comments.
    """
    def __init__(self, key, comments=True, **kwds):
        self.comments = comments
        super(Text, self).__init__(key, **kwds)

    def initial(self, text, comment):
        node = super(Text, self).initial(text, comment)
        self.parse(node, node.sectionText, comment)
        return node

    def parse(self, node, text, comment):
        if self.comments:
            if len(text) and len(comment):
                text += ' '
            text += comment
        if node.data:
            if len(node.data) and len(text):
                node.data += '\n'
            node.data += text
        else:
            node.data = text
        return True

    def merge(self, orig, new):
        """Just merge together subsequent entries."""
        if orig.data is None:
            orig.data = ""
        if len(orig.data):
            orig.data += '\n'
        orig.data += new.data

    def flatten(self, node):
        node.data = node.data.strip('\n')

class Section(Bracket):
    """Multi-line token, such as '[Model]'"""
    def __init__(self, key, pyparser=None, labeled=False, **kwds):
        """If a Section is labeled, the data is an OrderedDict of objects
           indexed by sectionText.
        """
        self.needs_text = labeled
        if labeled:
            kwds["initvalue"] = OrderedDict()
            self.merge = self.labeled_merge
        super(Section, self).__init__(key, pyparser, **kwds)

    def initial(self, text, comment):
        node = super(Section, self).initial(text, comment)
        if self.needs_text and not node.sectionText:
            raise Exception("Expected text after '{}'".format(text))
        elif not self.needs_text and node.sectionText:
            raise Exception("Unexpected text after keyword, '{}'".format(node.sectionText))
        node.key = node.sectionText
        return node

    def labeled_merge(self, orig, new):
        if new.key in orig.data:
            raise Exception("'{}' already contains '{}'".format(self.key, new.key))
        orig.data[new.key] = new.data

class TokenizeSection(Section):
    """Text from entire section is collected and parsed with pyparser."""
    def __init__(self, key, pyparser=None, **kwds):
        super(TokenizeSection, self).__init__(key, pyparser, **kwds)

    def initial(self, text, comment):
        node = super(TokenizeSection, self).initial(text, comment)
        node.text = ""
        return node

    def parse(self, node, text, comment):
        node.text += text
        node.text += '\n'
        return True

    def fin(self, node):
        node.data = self.pyparse(node.text)
        super(TokenizeSection, self).fin(node)

class TableSection(Section):
    """[<section name>] <col1 name> <col2 name> <...>
       <row name> <col1 data> <col2 data> <...>
    """
    def __init__(self, key, pyparser=None, headers=[], optional=[], **kwds):
        kwds["initvalue"] = OrderedDict()
        super(TableSection, self).__init__(key, pyparser, **kwds)
        self.needs_text = True
        self.parsers = OrderedDict()
        self.headers = [ key.lower() for key in headers ]
        self.optional = [ key.lower() for key in optional ]

    def initial(self, text, comment):
        node = super(TableSection, self).initial(text, comment)
        node.keys = node.sectionText.lower().split()
        if len(node.keys) != len(set(node.keys)):
            raise Exception("'{}' contains duplicates".format(node.sectionText))
        for key in self.headers:
            if key not in node.keys:
                raise Exception("Expected header names to contain '{}'".format(key))
        for key in node.keys:
            if key not in self.headers and key not in self.optional:
                raise Exception("Unexpected header name, '{}'".format(key))
        for key in self.optional:
            if key not in node.keys:
                node.keys.append(key)
        return node

    def assign_row(self, node, key, row):
        """For Seires_Pin_Mapping to override."""
        if key in node.data:
            raise Exception("'{}' already contains '{}'".format(self.key, key))
        node.data[key] = row

    def parse(self, node, text, comment):
        if super(TableSection, self).parse(node, text, comment):
            return True

        tokens = text.split()
        if not tokens:
            return True

        row = IBISNode(list(zip(node.keys, tokens[1:])))
        for name, parser in self.parsers.items():
            name = name.lower()
            if name in row:
                tmp = Node()
                try:
                    tmp.data = parser.parseString(row[name], parseAll=True)
                except ParseException as e:
                    raise Exception("Failed to parse '{}', '{}': {}".format(name, row[name], e.msg))
                self.flatten(tmp)
                row[name] = tmp.data
        for key in self.headers:
            if key not in row:
                raise Exception("Required column '{}' missing".format(key))
        for key in self.optional:
            if key not in row:
                row[key] = None

        self.assign_row(node, tokens[0], row)
        return True

    def add(self, obj):
        if isinstance(obj, ParserElement):
            self.parsers[obj.resultsName] = obj        
        else:
            super(Section, self).add(obj)

class ListSection(Section):
    """Each line is an element of a list."""
    def __init__(self, key, pyparser=None, **kwds):
        kwds["initvalue"] = list()
        super(ListSection, self).__init__(key, pyparser, **kwds)

    def parse(self, node, text, comment):
        if not super(ListSection, self).parse(node, text, comment):
            tmp = Node()
            tmp.data = self.pyparse(text)
            self.flatten(tmp)
            node.data.append(tmp.data)
        return True

class DictSection(Section):
    """Each line is indexed by the first token."""
    def __init__(self, key, pyparser=None, **kwds):
        kwds["initvalue"] = OrderedDict()
        super(DictSection, self).__init__(key, pyparser, **kwds)

    def parse(self, node, text, comment):
        if not super(DictSection, self).parse(node, text, comment):
            tokens = text.split(None, 1)
            tmp = Node()
            tmp.data = self.pyparse(tokens[1])
            self.flatten(tmp)
            if tokens[0] in node.data:
                raise Exception("'{}' already contains '{}'".format(self.key, tokens[0]))
            node.data[tokens[0]] = tmp.data
        return True

class RangeSection(Section):
    """Indexed tabular typ/min/max data, such as '[Pullup]'"""
    def __init__(self, key, increasing_idx=False, decreasing_idx=False, **kwds):
        self.increasing_idx = increasing_idx
        self.decreasing_idx = decreasing_idx
        super(RangeSection, self).__init__(key, RealRange(4), **kwds)

    def initial(self, text, comment):
        node = super(RangeSection, self).initial(text, comment)
        node.data = Range([ ( [], [] ), ( [], [] ), ( [], [] ) ])
        node.last_idx = None
        return node

    def parse(self, node, text, comment):
        if not super(RangeSection, self).parse(node, text, comment):
            tokens = self.pyparse(text)[0]
            if not node.last_idx:
                pass
            elif self.increasing_idx:
                if tokens[0] <= node.last_idx:
                    raise Exception("Index values not monotonic")
            elif self.decreasing_idx:
                if tokens[0] >= node.last_idx:
                    raise Exception("Index values not monotonic")
            elif tokens[0] > node.last_idx:
                self.increasing_idx = True
            elif tokens[0] < node.last_idx:
                self.decreasing_idx = True
            else:
                raise Exception("Index values not monotonic")
            node.last_idx = tokens[0]
            for i in range(3):
                if not tokens[i + 1] is None:
                    node.data[i][0].append(tokens[0])
                    node.data[i][1].append(tokens[i + 1])
        return True

    def flatten(self, node):
        pass

    def fin(self, node):
        for i in range(3):
            if i > 0 and not len(node.data[i][0]):
                node.data[i] = None
            elif len(node.data[i][0]) < 2:
                raise Exception("Requires at least 2 points")
        super(RangeSection, self).fin(node)


# FIXME: Make special object to encapsulate matrix with pin mapping.
class MatrixSection(Section):
    """Such as '[Resistance Matrix]'"""
    def __init__(self, key, pyparser=Real(), check=lambda f: f, **kwds):
        super(MatrixSection, self).__init__(key, pyparser, **kwds)
        self += Keyword("Bandwidth", Integer(check=positive))
        self += TokenizeSection("Row", labeled=True, pyparser=Word(printables) * (1,), asList=True)
        self.needs_text = True
        self.check = check

    def fin(self, node):
        try:
            pn = list(node.parent.parent.children.pin_numbers.keys())
        except:
            raise Exception("Could not find associated [Pin Numbers] section")
    
        try:
            count = node.parent.parent.children.number_of_pins
        except:
            raise Exception("Could not find associated [Number Of Pins] section")

        if count != len(pn):
            assert Exception("[Number Of Pins] does not match length of [Pin Numbers]")
    
        try:
            pm = node.parent.parent.children.pin_mapping
        except:
            # Create a 'Pin Mapping' child that maps pin names to matrix indexes.
            pm = dict((pin, i) for i, pin in enumerate(pn))
            node.parent.parent.children["Pin Mapping"] = pm
        node.data = zeros([len(pm), len(pm)])
        node.sectionText = node.sectionText.lower()

        for row, data in node.children.row.items():
            try:
                r = pm[row]
            except KeyError:
                raise Exception("Unknown pin name '{}' in matrix".format(row))

            if node.sectionText == "sparse_matrix":
                for col, val in zip(data[::2], data[1::2]):
                    try:
                        c = pm[col]
                    except KeyError:
                        raise Exception("Unknown pin name '{}' in matrix".format(col))
                    node.data[r][c] = self.check(ParseReal(val))

            elif node.sectionText == "banded_matrix" or node.sectionText == "full_matrix":
                c = r
                for val in data:
                    node.data[r][c] = self.check(ParseReal(val))
                    c = (c + 1) % len(pm)

            else:
                raise Exception("incomplete/unknown matrix".format(node.sectionText))

        node.children = None
        super(MatrixSection, self).fin(node)

class Param(Parse):
    """A single line param, such as 'Vinl 5.6', or 'Vinl_dc = 100mV'.
       'delim' is required if specified.
    """
    def __init__(self, key, pyparser=None, delim=None, **kwds):
        super(Param, self).__init__(key, pyparser, **kwds)
        self.delim = delim

    def can_parse(self, text):
        if text and text[0] != '[':
            if self.delim != None:
                name, d, _ = text.partition(self.delim)
                if d and name.strip().lower() == self.flat_key:
                    return True
            else:
                tokens = text.split(None, 1)
                if tokens[0].lower() == self.flat_key:
                    return True
        return False

    def initial(self, text, comment):
        node = super(Param, self).initial(text, comment)

        if self.delim != None:
            _, d, text = text.partition(self.delim)
        else:
            tokens = text.split(None, 1)
            text = tokens[1]
        node.data = self.pyparse(text)
        return node

class DictParam(Param):
    """Label indexing object as _key, such as 'Port_map'."""
    def __init__(self, key, pyparser, **kwds):
        kwds["initvalue"] = OrderedDict()
        super(DictParam, self).__init__(key, pyparser, **kwds)

    def flatten(self, new):
        new.key = self.pop(new, "_key")
        super(DictParam, self).flatten(new)

    def merge(self, orig, new):
        if new.key in orig.data:
            raise Exception("'{}' already contains '{}'".format(self.key, new.key))
        orig.data[new.key] = new.data

class RangeParam(Param):
    """Results are tagged with a _range, output is a typ, min, max,
       such as 'Corner'
    """
    def __init__(self, key, pyparser, **kwds):
        kwds["initvalue"] = Range([None, None, None])
        super(RangeParam, self).__init__(key, pyparser, **kwds)

    def flatten(self, new):
        if "_range" in new.data:
            new.range = self.pop(new, "_range").lower()
        else:
            new.range = "typ"
        super(RangeParam, self).flatten(new)

    def merge(self, orig, new):
        if new.range == "typ":
            r = 0
        elif new.range == "min":
            r = 1
        elif new.range == "max":
            r = 2
        else:
            raise Exception("Unknown range key, '{}'".format(new.range))

        if orig.data[r] is None:
            orig.data[r] = new.data
        else:
            raise Exception("{} value for '{}' already specified".format(new.range, self.key))

class RangeDictParam(Param):
    """Oi...we need a combination of the above two, as in 'D_to_A'."""
    def __init__(self, key, pyparser, **kwds):
        kwds["initvalue"] = OrderedDict()
        super(RangeDictParam, self).__init__(key, pyparser, **kwds)

    def flatten(self, new):
        new.key = self.pop(new, "_key")
        if "_range" in new.data:
            new.range = self.pop(new, "_range").lower()
        else:
            new.range = "typ"
        super(RangeDictParam, self).flatten(new)

    def merge(self, orig, new):
        if new.key not in orig.data:
            orig.data[new.key] = Range([None, None, None])
        
        if new.range == "typ":
            r = 0
        elif new.range == "min":
            r = 1
        elif new.range == "max":
            r = 2
        else:
            raise Exception("Unknown range key, '{}'".format(new.range))

        if orig.data[new.key][r] is None:
            orig.data[new.key][r] = new.data
        else:
            raise Exception("{} value for '{}' already specified".format(new.range, new.key))

class Header(Parse):
    """Section 4."""
    def __init__(self, **kwds):
        kwds["required"] = True
        super(Header, self).__init__("header", **kwds)
        self += Keyword("IBIS Ver", required=True)
        self += Keyword("File Name", required=True)
        self += Keyword("File Rev", required=True)
        self += Keyword("Date")
        self += Text("Source")
        self += Text("Notes")
        self += Text("Disclaimer")
        self += Text("Copyright")

class Series_Pin_Mapping(TableSection):
    def __init__(self):
        super(Series_Pin_Mapping, self).__init__("Series Pin Mapping",
            headers=["pin_2", "model_name"], optional=["function_table_group"])

    def assign_row(self, node, key, row):
        key = (key, row["pin_2"] )
        del row["pin_2"]
        if key[0] == key[1]:
            raise Exception("series pin '{}' maps to itself".format(key[0]))
        super(Series_Pin_Mapping, self).assign_row(node, key, row)

class Component(Section):
    """Section 5."""
    def __init__(self, **kwds):
        kwds["required"] = True
        kwds["labeled"] = True
        super(Component, self).__init__("Component", **kwds)

        package = Section("Package", required=True)
        package += Param("R_pkg", RealRange(), required=True)
        package += Param("L_pkg", RealRange(), required=True)
        package += Param("C_pkg", RealRange(), required=True)

        pin = TableSection("Pin", required=True,
            headers=["signal_name", "model_name"],
            optional=["R_pin", "L_pin", "C_pin"])
        pin += NAReal(check=positive)("R_pin")
        pin += NAReal(check=positive)("L_pin")
        pin += NAReal(check=positive)("C_pin")

        alternate_package_models = ListSection("Alternate Package Models")
        alternate_package_models += End("End Alternate Package Models")

        diff_pin = TableSection("Diff Pin",
            headers=["inv_pin", "vdiff", "tdelay_typ"],
            optional=["tdelay_min", "tdelay_max"])
        diff_pin += NAReal(check=positive)("vdiff")
        diff_pin += NAReal()("tdelay_typ")
        diff_pin += NAReal()("tdelay_min")
        diff_pin += NAReal()("tdelay_max")

        switch_group = oneof("On Off") + Word(alphanums) * (0,) + Literal("/").suppress()

        series_switch_groups = TokenizeSection("Series Switch Groups",
            Group(switch_group) * (0,), asList=True)

        self += Param("Si_location", oneof("Pin Die"))
        self += Param("Timing_location", oneof("Pin Die"))
        self += Keyword("Manufacturer", required=True)
        self += package
        self += pin
        self += Keyword("Package Model")
        self += alternate_package_models
        self += TableSection("Pin Mapping",
            headers=["pulldown_ref", "pullup_ref"],
            optional=["gnd_clamp_ref", "power_clamp_ref", "ext_ref"])
        self += diff_pin
        self += Series_Pin_Mapping()
        self += series_switch_groups
        self += Node_Declarations()
        self += Circuit_Call()
        self += EMI_Component() 

    def fin(self, node):
        super(Component, self).fin(node)

        # NB: Even though 'POWER', 'GND', and 'NC' are 'reserverd words' and
        # therefore case sensitive, the golden parser doesn't care.
        for name, pin in node.children.pin.items():
            if pin.model_name.upper() in [ "POWER", "GND", "NC", "CIRCUITCALL" ]:
                pin.model_name = pin.model_name.upper()
            if pin.model_name == "NC":
                pin.model_name = None

        if "Pin Mapping" in node.children:
            # Make sure all pins under both pin and Pin Mapping 
            if len(node.children.pin) != len(node.children.pin_mapping):
                raise Exception("Pin mapping table is incomplete")

            # Make sure the bus mappings line up properly
            available_bus = set()
            used_bus = set()
            for pin_name, mapping in node.children.pin_mapping.items():
                for ref in [ "pullup_ref", "pulldown_ref", "gnd_clamp_ref", "power_clamp_ref", "ext_ref" ]:
                    if mapping[ref] and mapping[ref].upper() == "NC":
                        mapping[ref] = None

                if pin_name not in node.children.pin:
                    raise Exception("Invalid pin, '{}', listed in Pin Mapping".format(pin_name))
                pin_info = node.children.pin[pin_name]

                if pin_info.model_name == "GND":
                    if mapping.pullup_ref is not None:
                        raise Exception("Expected 'NC' for pin mapping of '{}'".format(pin_name))
                    if mapping.pulldown_ref is None:
                        raise Exception("Unexpected 'NC' for pin mapping of '{}'".format(pin_name))
                    available_bus.add(mapping.pulldown_ref)

                elif pin_info.model_name == "POWER":
                    if mapping.pulldown_ref is not None:
                        raise Exception("Expected 'NC' for pin mapping of '{}'".format(pin_name))
                    if mapping.pullup_ref is None:
                        raise Exception("Unexpected 'NC' for pin mapping of '{}'".format(pin_name))
                    available_bus.add(mapping.pullup_ref)

                else:
                    for bus in list(mapping.values()):
                        if bus is not None:
                            used_bus.add(bus)

            missing = used_bus.difference(available_bus)
            if len(missing):
                raise Exception("'{}' listed in pin mapping, but not available".format(missing))

        if "Diff Pin" in node.children:
            for pin_name, mapping in node.children.diff_pin.items():
                if pin_name not in node.children.pin:
                    raise Exception("Invalid pin, '{}', listed in Diff Pin".format(pin_name))
                if mapping.inv_pin not in node.children.pin:
                    raise Exception("Invalid pin, '{}', listed in Diff Pin".format(mapping.inv_pin))
                if pin_name == mapping.inv_pin:
                    raise Exception("Diff Pin '{}' maps to itself".format(pin_name))

                # Fill in vdiff default
                if mapping.vdiff is None:
                    mapping.vdiff = 200e-3

                # Create tdelay Range and fill in typ/min defaults
                tdelay = Range([])
                for t in [ "tdelay_typ", "tdelay_min", "tdelay_max" ]:
                    tdelay.append(mapping[t])
                    del mapping[t]
                if tdelay[0] is None: tdelay[0] = 0
                if tdelay[1] is None: tdelay[1] = 0
                mapping["tdelay"] = tdelay

        function_table_groups = set()
        if "Series Pin Mapping" in node.children:
            for pin_tuple, mapping in node.children.series_pin_mapping.items():
                for pin_name in pin_tuple:
                    if pin_name not in node.children.pin:
                        raise Exception("Invalid pin, '{}', listed in series pin mapping"
                            .format(pin_name))
                group = mapping.function_table_group
                if group is not None:
                    function_table_groups.add(group)

        if function_table_groups:
            if not "Series Switch Groups" in node.children:
                raise Exception("Series Pin Mapping has function_table_group elements, "
                    "but no Series Switch Groups exist")
            for group in node.children.series_switch_groups:
                groups = set(group[1:])
                missing = groups.difference(function_table_groups)
                if len(missing):
                    raise Exception("Series Switch Groups list '{}' which are not "
                        "available in the series pin mapping table"
                        .format(missing))
        elif "Series Switch Groups" in node.children:
            raise Exception("Series Switch Groups exists but no function_table_group "
                "elements exist under series pin mapping")

        if "Circuit Call" in node.children:
            for name, call in node.children.circuit_call.items():
                for n in [ "signal_pin", "diff_signal_pins", "series_pins" ]:
                    if n in call:
                        obj = call[n]
                        if not isinstance(obj, list):
                            obj = [ obj ]
                        for pin in obj:
                            if pin not in node.children["pin"]:
                                raise Exception("Circuit Call '{}' contains unknown pin '{}'"
                                    .format(name, pin))

        if "Begin EMI Component" in node.children:
            if "Pin EMI" in node.children.begin_emi_component:
                for pin, item in emi_component.pin_emi.items():
                    if pin not in node.children.pin:
                        raise Exception("EMI Component lists unknown pin '{}'"
                            .format(pin))

class BaseModel(Section):
    """Section 6/6a."""
    def __init__(self, key, **kwds):
        kwds["labeled"] = True
        super(BaseModel, self).__init__(key, **kwds)

        # FIXME: check required for submodel types
        ramp = Section("Ramp")
        ramp += Param("dV/dt_r", RampRange())
        ramp += Param("dV/dt_f", RampRange())
        ramp += Param("R_load", Real(check=positive), delim='=', default=50)

        # FIXME: Check for monotonic
        for s in [ "Pullup", "Pulldown", "GND Clamp", "POWER Clamp" ]:
            self += RangeSection(s)

        for s in [ "Rising Waveform", "Falling Waveform" ]:
            table = RangeSection(s, data_name="waveform", increasing_idx=True, list_merge=True)
            table += Param("R_fixture", Real(check=positive), delim='=', required=True)
            table += Param("V_fixture", Real(), delim='=', required=True)
            for p in [ "V_fixture_min", "V_fixture_max" ]:
                table += Param(p, Real(), delim='=')
            for p in [ "C_fixture", "L_fixture", "R_dut", "L_dut", "C_dut" ]:
                table += Param(p, Real(check=positive), delim='=', default=0)
            table += RangeSection("Composite Current")
            self += table

        self += ramp

    def fin(self, node):
        super(BaseModel, self).fin(node)

        # Make v_fixture a range, axe _min, _max
        for s in [ "Rising Waveform", "Falling Waveform" ]:
            if s in node.children:
                for waveform in node.children[s]:
                    r = Range([waveform.V_fixture])
                    for m in [ "V_fixture_min", "V_fixture_max" ]:
                        if m in waveform:
                            r.append(waveform[m])
                            del waveform[m]
                        else:
                            r.append(None)
                    waveform.V_fixture = r

class Series_MOSFET(RangeSection):
    def __init__(self):
        super(Series_MOSFET, self).__init__("Series MOSFET", initvalue=OrderedDict())
        self += Param("Vds", Real(), delim='=')

    def flatten(self, node):
        try:
            node.key = node.children.vds
        except:
            raise Exception("Series MOSFET missing Vds")
        node.children = None
        super(Series_MOSFET, self).flatten(node)

    def merge(self, orig, new):
        if new.key in orig.data:
            raise Exception("'{}' already contains '{}'".format(self.key, new.key))
        orig.data[new.key] = new.data

class Model(BaseModel):
    """Section 6."""
    def __init__(self, **kwds):
        # NB: The spec claims that Model is required, but ibischk5 does not care.
        # Really it should only be required if there is a referenced model
        # kwds["required"] = True
        super(Model, self).__init__("Model", **kwds)

        model_spec = Section("Model Spec")
        for p in ["Vinh", "Vinl", "Vinh+", "Vinh-", "Vinl+", "Vinl-",
                "S_overshoot_high", "S_overshoot_low",
                "D_overshoot_high", "D_overshoot_low",
                "D_overshoot_ampl_h", "D_overshoot_ampl_l",
                "Pulse_high", "Pulse_low", "Vmeas", "Vref",
                "Vref_rising", "Vref_falling", "Vmeas_rising", "Vmeas_falling" ]:
            model_spec += Param(p, RealRange())
        for p in [ "D_overshoot_time", "D_overshoot_area_h", "D_overshoot_area_l",
                "Pulse_time", "Vref", "Cref", "Rref", "Cref_rising", "Cref_falling",
                "Rref_rising", "Rref_falling", "Rref_diff", "Cref_diff"]:
            model_spec += Param(p, RealRange(check=positive))

        receiver_thresholds = Section("Receiver Thresholds")
        for p in [ "Vth", "Vth_min", "Vth_max",
                "Vinh_ac", "Vinh_dc", "Vinl_ac", "Vinl_dc", "Threshold_sensitivity",
                "Vcross_low", "Vcross_high",
                "Vdiff_ac", "Vdiff_dc", "Tslew_ac", "Tdiffslew_ac" ]:
            receiver_thresholds += Param(p, Real(), delim='=')
        receiver_thresholds += Param("Reference_supply",
            oneof("Power_clamp_ref Gnd_clamp_ref Pullup_ref Pulldown_ref Ext_ref"))

        driver_schedule = DictSection("Driver Schedule",
            NAReal()("Rise_on_dly") - NAReal()("Rise_off_dly") -
            NAReal()("Fall_on_dly") - NAReal()("Fall_off_dly"), asDict=True)

        receiver_thresholds = Section("Receiver Thresholds")
        for p in [ "Vth", "Vth_min", "Vth_max",
                "Vinh_ac", "Vinh_dc", "Vinl_ac", "Vinl_dc", "Threshold_sensitivity",
                "Vcross_low", "Vcross_high",
                "Vdiff_ac", "Vdiff_dc", "Tslew_ac", "Tdiffslew_ac" ]:
            receiver_thresholds += Param(p, Real(), delim='=')
        receiver_thresholds += Param("Reference_supply",
            oneof("Power_clamp_ref Gnd_clamp_ref Pullup_ref Pulldown_ref Ext_ref"))

        self += Param("Model_type",
            oneof("Input Output I/O 3-state Open_drain I/O_open_drain " +
                  "Open_sink I/O_open_sink Open_source I/O_open_source " +
                  "Input_ECL Output_ECL I/O_ECL 3-state_ECL " +
                  "Input_diff Output_diff I/O_diff 3-state_diff " +
                  "Series Series_switch Terminator"), required=True)
        self += Param("Polarity", oneof("Non-Inverting Inverting"))
        self += Param("Enable", oneof("Active-High Active-Low"))
        for p in [ "Vinl", "Vinh", "Vmeas", "Vref" ]:
            self += Param(p, Real(), delim='=')
        for p in [ "Cref", "Rref", "Rref_diff", "Cref_diff" ]:
            self += Param(p, Real(check=positive), delim='=')
        for p in [ "C_comp", "C_comp_pullup", "C_comp_pulldown",
                "C_comp_power_clamp", "C_comp_gnd_clamp" ]:
            self += Param(p, RealRange(check=positive))
        self += Keyword("Temperature Range", RealRange(), default=Range([50, 0, 100]))
        for p in [ "Voltage Range",
                "POWER Clamp Reference", "GND Clamp Reference",
                "External Reference", "TTgnd", "TTpower",
                "Pullup Reference", "Pulldown Reference" ]:
            self += Keyword(p, RealRange())
        for p in [ "R Series", "L Series", "Rl Series",
                "C Series", "Lc Series", "Rc Series",
                "Rgnd", "Rpower", "Rac", "Cac" ]:
            self += Keyword(p, RealRange(check=positive))
        for p in [ "ISSO PD", "ISSO PU" ]:
            self += RangeSection(p)
        self += EMI_Model()

        for s in [ "On", "Off" ]:
            series = Section(s)
            series += RangeSection("Series Current")
            series += Series_MOSFET()
            for p in [ "R Series", "L Series", "Rl Series",
                    "C Series", "Lc Series", "Rc Series" ]:
                series += Keyword(p, RealRange(check=positive))
            self += series

        self += model_spec
        self += receiver_thresholds
        self += DictSection("Add Submodel", oneof("Driving Non-Driving All"))
        self += driver_schedule
        self += External_Model()
        self += Algorithmic_Model()
        self += RangeSection("Series Current")
        self += Series_MOSFET()
        self += receiver_thresholds

    def fin(self, node):
        super(Model, self).fin(node)

        # Drop old drain naming
        model_type = node.children.model_type
        if model_type == "open_drain":
            model_type = "open_sink"
            node.children.model_type = model_type
        elif model_type == "i/o_open_drain":
            model_type = "i/o_open_sink"
            node.children.model_type = model_type

        type_diff = model_type.endswith("diff")
        type_ecl = model_type.endswith("ecl")
        type_source = model_type.endswith("source")
        type_sink = model_type.endswith("sink")
        type_output = model_type.startswith(("i/o", "output", "3-state", "open"))
        type_input = model_type.startswith(("i/o", "input"))
        type_series = model_type.startswith("series")
        type_enable = model_type.startswith(("i/o", "3-state"))
        type_switch = model_type == "series_switch"
        type_terminator = model_type == "terminator"

        required = []
        disallowed = []
        if type_diff:
            required += ["External Model"]
        elif type_input:
            # Fill in Vinl/Vinh defaults
            if type_ecl:
                defaults = [ [ "Vinl", -1.475 ], [ "Vinh", -1.165 ] ]
            else:
                defaults = [ [ "Vinl", 0.8 ], [ "Vinh", 2.0 ] ]
            for n, v in defaults:
                if n not in node.children or node.children[n] is None:
                    print("Warning: '{}' for model '{}' not defined, using '{}'".format(n, node.key, v))
                    node.children[n] = v

        if not type_input:
            disallowed += ["Receiver Thresholds"]

        if type_series and not type_switch:
            disallowed += ["Add Submodel"]
            if "L Series" not in node.children:
                disallowed += ["Rl Series"]
            elif "Rl Series" not in node.children:
                node.children["Rl Series"] = 0.0
            if "C Series" not in node.children:
                disallowed += [ "Rc Series", "Lc Series" ]
            else:
                if "Rc Series" not in node.children:
                    node.children["Rc series"] = 0.0
                if "Lc Series" not in node.children:
                    node.children["Lc Series"] = 0.0
        else:
            disallowed += [ "R Series", "L Series", "Rl Series", "C Series",
                "Lc Series", "Rc Series", "Series Current", "Series Mosfet" ]

        if type_switch:
            required += [ "On", "Off" ]
            disallowed += ["Add Submodel"]
        else:
            disallowed += [ "On", "Off" ]

        if not type_terminator:
            disallowed += [ "Rgnd", "Rpower", "Rac", "Cac" ]

        if "Rac" in node.children != "Cac" in node.children:
            raise Exception("'Rac' and 'Cac' must be specified together")

        if type_output and not "External Model" in node.children:
            required += ["Ramp"]

        if "Add Submodel" in node.children:
            for submodel_name, mode in node.children.add_submodel.items():
                if mode == "driving" and type_output:
                    raise Exception("Type '{}' cannot have submodel '{}' of mode '{}'"
                        .format(model_type, submodel_name, mode))
                if mode == "non-driving" and not type_input and not type_enable:
                    raise Exception("'Type '{}' cannot have submodel '{}' of mode '{}'"
                        .format(model_type, submodel_name, mode))

        if "Model Spec" in node.children:
            for level in [ "low", "high" ]:
                if "D_overshoot_" + level in node.children.model_spec:
                    if "S_overshoot_" + level not in node.children.model_spec:
                        raise Exception("D_overshoot_{} requires S_overshoot_{}"
                            .format(level, level))
                    if "D_overshoot_time" not in node.children.model_spec:
                        raise Exception("D_overshoot_{} requires D_overshoot_time"
                            .format(level))
                if "D_overshoot_area_" + level[0] in node.children:
                    if "D_overshoot_ampl_" + level[0] not in node.chidren:
                        raise Exception("D_overshoot_area_{} requires D_overshoot_ampl_{}"
                            .format(level[0], level[0]))

        if "Receiver Thresholds" in node.children:
            thresh = node.children.receiver_thresholds
            if "Threshold_sensitivity" in thresh:
                if "Reference_supply" not in thresh:
                    raise Exception("'Reference_supply' required when 'Threshold_sensitivity' is present")
                required += ["{}erence".format(thresh.reference_supply)]

        if "Driver Schedule" in node.children:
            for driver, data in node.children.driver_schedule.items():
                count = 0
                for name, val in data.items():
                    if val and not val >= 0:
                        raise Exception("'{}' driver schedule item '{}' has invalid value '{}'"
                            .format(driver, name, val))
                    if val is not None:
                        count += 1
                invalid = False
                if count == 2:
                    print(data)
                    for combination in [ [ "rise_on_dly", "fall_off_dly" ],
                            [ "rise_off_dly", "fall_on_dly" ] ]:
                        if combination[0] in data and combination[1] in data:
                            invalid = True
                if (count != 2 and count != 4) or invalid:
                    raise Exception("Invalid dly combination for driver schedule '{}'"
                        .format(driver))

        if "Voltage Range" not in node.children:
            required += [ "Pullup Reference", "Pulldown Reference",
                "POWER Clamp Reference", "GND Clamp Reference" ]

        c_comp_required = True
        for keyword in [ "pullup", "pulldown", "power_clamp", "gnd_clamp" ]:
            if "C_comp_{}".format(keyword) in node.children:
                c_comp_required = False
                break
        if "External Model" in node.children:
            c_comp_required = False
            ports = set()
            if type_input:
                ports.add("D_receive")
            if type_output:
                ports.add("D_drive")
            if type_enable:
                ports.add("D_enable")
            if type_switch:
                ports.add("D_switch")

            if type_diff:
                ports |= set([ "A_signal_pos", "A_signal_neg" ])
            elif type_series:
                ports |= set(["A_pos", "A_neg"])
            else:
                ports.add("A_signal")

            ext = node.children.external_model
            used = set(ext.ports)
            for t in ["D_to_A", "A_to_D"]:
                if t in ext:
                    for name, obj in ext[t].items():
                        used.add(name)
                        for port in ["port1", "port2"]:
                            used.add(obj[port])
            for port in ports:
                if port not in used:
                    raise Exception("External Model missing port '{}'"
                        .format(port))

        if c_comp_required:
            required += ["C_comp"]

        for keyword in disallowed:
            if keyword in node.children:
                raise Exception("Keyword '{}' not permitted in type '{}'"
                    .format(keyword, model_type))
        for keyword in required:
            if keyword not in node.children:
                raise Exception("Type '{}' missing required keyword '{}'"
                    .format(model_type, keyword))
            
        if type_switch:
            for d in [ "On", "Off"]:
                disallowed = []
                sect = node.children[d]
                if "L Series" not in sect:
                    disallowed += ["Rl Series"]
                elif "Rl Series" not in sect:
                    sect["Rl Series"] = 0.0
                if "C Series" not in sect:
                    disallowed += [ "Rc Series", "Lc Series" ]
                else:
                    if "Rc Series" not in sect:
                        sect["Rc Series"] = 0.0
                    if "Lc Series" not in sect:
                        sect["Lc Series"] = 0.0
                for keyword in disallowed:
                    if keyword in sect:
                        raise Exception("Keyword '{}' not permitted in '{}'"
                            .format(keyword, d))

class Test_Data(Section):
    """See http://www.vhdl.org/pub/ibis/birds/bird142.txt"""
    def __init__(self, **kwds):
        kwds["labeled"] = True
        super(Test_Data, self).__init__("Test Data", **kwds)

        self += Param("Test_data_type", oneof("Single_ended Differential"), required=True)
        self += Param("Driver_model", required=True)
        self += Param("Driver_model_inv")
        self += Param("Test_load", required=True)
        for s in [ "Rising Waveform Near", "Falling Waveform Near",
                "Rising Waveform Far", "Falling Waveform Far",
                "Diff Rising Waveform Near", "Diff Falling Waveform Near",
                "Diff Rising Waveform Far", "Diff Falling Waveform Far" ]:
            self += RangeSection(s, increasing_idx=True)

    def fin(self, node):
        super(Test_Data, self).fin(node)

        if "Driver_model_inv" in node.children:
            if node.children.test_data_type != "differential":
                raise Exception("Contains Driver_model_inv but is not differential")
        for s in [ "Rising Waveform Near", "Falling Waveform Near",
                "Rising Waveform Far", "Falling Waveform Far",
                "Diff Rising Waveform Near", "Diff Falling Waveform Near",
                "Diff Rising Waveform Far", "Diff Falling Waveform Far" ]:
            if s in node.children:
                has = True
                break
        if not has:
            raise Exception("Does not have any waveforms")

class Test_Load(Section):
    def __init__(self, **kwds):
        kwds["labeled"] = True
        super(Test_Load, self).__init__("Test Load", **kwds)

        self += Param("Test_load_type", oneof("Single_ended Differential"), required=True)
        self += Param("V_term1", RealRange())
        self += Param("V_term2", RealRange())
        self += Param("Receiver_model")
        self += Param("Receiver_model_inv")
        for p in [ "C1_near", "Rs_near", "Ls_near",
                "C2_near", "Rp1_near", "Rp2_near", "Td", "Zo", "Rp1_far", "Rp2_far",
                "C2_far", "Ls_far", "Rs_far", "C1_far",
                "R_diff_near", "R_diff_far" ]:
            self += Param(p, Real(check=positive), delim='=')

    def fin(self, node):
        super(Test_Load, self).fin(node)

        if "Td" in node.children and not "Zo" in node.children:
            raise Exception("Contains Td but not Zo")

        if "Receiver_model_inv" in node.children:
            if node.children.test_load_type != "differential":
                # Must be ignored, can contain garbage.
                del node.children["Receiver_model_inv"]

        for n in [ "1", "2" ]:
            for t in [ "near", "far" ]:
                if "Rp{}_{}".format(n, t) in node.children:
                    if "V_term{}".format(n) not in node.children:
                        raise Exception("Contains 'Rp{}_{}' but not 'V_term{}'"
                            .format(n, t, n))

class Submodel(BaseModel):
    """Section 6a."""
    def __init__(self, **kwds):
        super(Submodel, self).__init__("Submodel", **kwds)

        submodel_spec = Section("Submodel Spec")
        submodel_spec += Param("V_trigger_r", RealRange())
        submodel_spec += Param("V_trigger_f", RealRange())
        submodel_spec += Param("Off_delay", RealRange(check=positive))

        self += Param("Submodel_type", oneof("Dynamic_clamp Bus_hold Fall_back"), required=True)
        for s in [ "GND Pulse Table", "POWER Pulse Table" ]:
            self += RangeSection(s, increasing_idx=True)
        self += submodel_spec

    def fin(self, node):
        super(Submodel, self).fin(node)

        submodel_type = node.children.submodel_type
        try:
            spec = node.children.submodel_spec
        except:
            spec = None
        if submodel_type == "dynamic_clamp":
            if "GND Pulse Table" in node.children:
                if not spec or not "V_trigger_f" in spec:
                    raise Exception("Missing 'V_trigger_f' table")
            if "POWER Pulse Table" in node.children:
                if not spec or not "V_trigger_r" in spec:
                    raise Exception("Missing 'V_trigger_r' table")
        elif submodel_type == "fall_back":
            if "Pullup" in node.children and "Pulldown" in node.children:
                raise Exception("Cannot have both 'Pullup' and 'Pulldown'")
            if spec and "Off_delay" in spec:
                raise Exception("Contains disallowed keyword, 'Off_delay'")
        if submodel_type in [ "fall_back", "bus_hold" ]:
            if not "Pullup" in node.children and not "Pulldown" in node.children:
                raise Exception("Needs 'Pullup' or 'Pulldown'")
            if not "Ramp" in node.children:
                raise Exception("Missing 'ramp' keyword")
            for t in [ "V_trigger_f", "V_trigger_r" ]:
                if not spec or t not in spec:
                    raise Exception("Missing '{}' keyword".format(t))

class External_Common(Section):
    """"Section 6b."""
    def __init__(self, key, **kwds):
        super(External_Common, self).__init__(key, **kwds)

        corner = RangeParam("Corner",
            oneof("Typ Min Max")("_range") -
            Word(printables)("file_name") - Word(printables)("circuit_name"), asDict=True, required=True)

        d_to_a = RangeDictParam("D_to_A", Word(printables)("_key") -
            Word(printables)("port1") - Word(printables)("port2") -
            Real()("vlow") - Real()("vhigh") -
            Real(check=positive)("trise") - Real(check=positive)("tfall") -
            Optional(oneof("Typ Min Max")("_range")), asDict=True)

        a_to_d = RangeDictParam("A_to_D", Word(printables)("_key") -
            Word(printables)("port1") - Word(printables)("port2") -
            Real()("vlow") - Real()("vhigh") -
            Optional(oneof("Typ Min Max")("_range")), asDict=True)

        self += Param("Language",
            oneof("SPICE VHDL-AMS Verilog-AMS VHDL-A(MS) Verilog-A(MS)"))
        self += corner
        self += Param("Parameters", Word(printables) * (0,), asList=True, list_merge=True)
        self += Param("Ports", Word(printables) * (0,), asList=True, list_merge=True, required=True)
        self += d_to_a
        self += a_to_d
        self += End("End " + key) 

    def fin(self, node):
        super(External_Common, self).fin(node)

        if node.children.corner[0] is None:
            raise Exception("Missing typ corner '{}'".format(name))
        new_obj = node.children.corner[0]
        for n in [ "file_name", "circuit_name" ]:
            new_obj[n] = Range([ new_obj[n] ])
            for i in [ 1, 2 ]:
                if node.children.corner[i] is not None:
                    new_obj[n].append(node.children.corner[i][n])
                else:
                    new_obj[n].append(None)
        node.children.corner = new_obj
        for typ in [ "D_to_A", "A_to_D" ]:
            if typ in node.children:
                for name, obj in node.children[typ].items():
                    if obj[0] is None:
                        raise Exception("Missing typ '{}' '{}'".format(typ, name))
                    new_obj = obj[0]
                    for n in [ "vlow", "vhigh", "trise", "tfall" ]:
                        if n in new_obj:
                            new_obj[n] = Range([ new_obj[n] ])
                            for i in [ 1, 2 ]:
                                if obj[i] is not None:
                                    new_obj[n].append(obj[i][n])
                                else:
                                    new_obj[n].append(None)
                    for i in [ 1, 2 ]:
                        for n in [ "port1", "port2" ]:
                            if obj[i] is not None and obj[i][n] != obj[0][n]:
                                raise Exception("D_to_A '{}' typ/min/max '{}' do not match"
                                    .format(name, n))
                    node.children[typ][name] = new_obj


class External_Circuit(External_Common):
    def __init__(self, **kwds):
        kwds["labeled"] = True
        super(External_Circuit, self).__init__("External Circuit", **kwds)

class External_Model(External_Common):
    def __init__(self, **kwds):
        super(External_Model, self).__init__("External Model", **kwds)

    def fin(self, node):
        super(External_Model, self).fin(node)

        if "D_to_A" in node.children:
            for name, d_to_a in node.children.d_to_a.items():
                if name not in ["D_drive", "D_enable", "D_switch"]:
                    raise Exception("Contains disallowed port '{}' in D_to_A".format(name))

        if "A_to_D" in node.children:
            for name, a_to_d in node.children.a_to_d.items():
                if name != "D_receive":
                    raise Exception("Contains disallowed port '{}' in A_to_D".format(name))

class Node_Declarations(TokenizeSection):
    def __init__(self, **kwds):
        pyparser = Word(printables) * (0,)
        kwds["asList"] = True
        super(Node_Declarations, self).__init__("Node Declarations", pyparser=pyparser, **kwds)
        self += End("End Node Declarations")

class Circuit_Call(Section):
    def __init__(self, **kwds):
        kwds["labeled"] = True
        super(Circuit_Call, self).__init__("Circuit Call", **kwds)

        self += Param("Signal_pin")
        self += Param("Diff_signal_pins", Word(printables) * 2, asList=True)
        self += Param("Series_pins", Word(printables) * 2, asList=True)
        self += DictParam("Port_map", Word(printables)("_key") - Word(printables))
        self += End("End Circuit Call")

    def fin(self, node):
        super(Circuit_Call, self).fin(node)
        count = 0
        for n in [ "Signal_pin", "Diff_signal_pins", "Series_pins"]:
            if n in node.children:
                count += 1
                obj = node.children[n]
                if isinstance(obj, list):
                    if obj[0] == obj[1]:
                        raise Exception("'{}' points to itself".format(n))

        if count > 1:
            raise Exception("Has more than one pin mapping")

class Algorithmic_Model(Section):
    """Section 6c."""
    def __init__(self, **kwds):
        super(Algorithmic_Model, self).__init__("Algorithmic Model", **kwds)

        self += Param("Executable",
            Word(printables)("Platform_Compiler_Bits") -
            Word(printables)("File_Name") -
            Word(printables)("Parameter_File"), asDict=True, list_merge=True)
        self += End("End Algorithmic Model")

def stubKeyword(key):
    return Suppress(CaselessKeyword(key) + Literal("=")) + Real(check=positive)(key)

class Pin_Numbers(TokenizeSection):
    """The parse engine needs to changed based on whether or not there is a
       'Number Of Sections' keyword, otherwise the grammar is ambiguous.
    """
    def __init__(self, **kwds):
        super(Pin_Numbers, self).__init__("Pin Numbers", **kwds)

        stubs = Forward()

        fork_desc = (Suppress(LineStart() + CaselessKeyword("Fork") + LineEnd()) +
            Group(stubs * (0,)).setParseAction(lambda f: f.asList()) +
            Suppress(LineStart() + CaselessKeyword("Endfork") + LineEnd()))

        stub_desc = (
            stubKeyword("Len") +
            Each([ Optional(stubKeyword(k)) for k in [ "L", "R", "C" ] ]) -
            Suppress(Literal("/") + LineEnd() * (0,))
        ).setParseAction(lambda f: IBISNode(f.asDict()))

        stubs << (stub_desc | fork_desc)

        self.stub_parser = Group(Suppress(LineStart()) + Word(printables) - ~LineEnd() -
                Group(stubs * (1,)).setParseAction(lambda f: f.asList())) * (1,)
        self.stub_parser.setDefaultWhitespaceChars(" \t\r")
        self.stub_parser.setParseAction(orderedDict)

        self.plain_parser = (Suppress(LineEnd() * (0,)) + Word(printables) - Suppress(LineEnd() * (1,))) * (0,)
        self.plain_parser.setParseAction(lambda f: OrderedDict([(g, None) for g in f]))

    def fin(self, node):
        # FIXME: Check Number Of Sections number.
        # FIXME: Give more sensible error if 'Number Of Sections' is after 'Pin Numbers'.
        if "Number Of Sections" in node.parent.children:
            self.pyparser = self.stub_parser
        else:
            self.pyparser = self.plain_parser
        super(Pin_Numbers, self).fin(node)

class Package_Model(Section):
    """Section 7."""
    def __init__(self, **kwds):
        kwds["labeled"] = True
        super(Package_Model, self).__init__("Define Package Model", **kwds)

        model_data = Section("Model Data")
        for s in [ "Inductance Matrix", "Capacitance Matrix" ]:
            model_data += MatrixSection(s)
        # NB: Resistance Matrix values can only be positive, but ibischk5 does not
        # even give a warning, much less an error on negative values. Because
        # models exist in the wild with this issue, ignore for now until we have
        # warning support.
        model_data += MatrixSection("Resistance Matrix")
        model_data += End("End Model Data")

        self += Keyword("Manufacturer", required=True)
        self += Keyword("OEM", required=True)
        self += Keyword("Description", required=True)
        self += Keyword("Number Of Sections", Integer(check=positive))
        self += Keyword("Number Of Pins", Integer(check=positive), required=True)
        self += Pin_Numbers()
        self += model_data
        self += End("End Package Model")

    def fin(self, node):
        super(Package_Model, self).fin(node)

        if "Model Data" in node.children and "Number Of Sections" in node.children:
            raise Exception("'Model Data' and 'Number Of Sections' are mutually exclusive")

        if "Model Data" not in node.children and "Number Of Sections" not in node.children:
            raise Exception("Require 'Model Data' or 'Number Of Sections'")

        if node.children.number_of_pins != len(node.children.pin_numbers):
            raise Exception("'Number Of Pins' does not match count of 'Pin Numbers'")

class Path_Description(TokenizeSection):
    def __init__(self, **kwds):
        kwds["labeled"] = True
        kwds["asList"] = True
        kwds["required"] = True

        paths = Forward()

        fork_desc = Group(Suppress(CaselessKeyword("Fork")) +
            (paths * (0,)).setParseAction(lambda f: f.asList()) +
            Suppress(CaselessKeyword("Endfork")))

        path_pin = (Suppress(CaselessKeyword("Pin")) - Word(printables)("pin"))
        path_pin.setParseAction(lambda f: IBISNode(f.asDict()))

        path_node = Suppress(CaselessKeyword("Node")) - Word(printables)("node")
        path_node.setParseAction(lambda f: IBISNode(f.asDict()))

        stub_desc = (
            stubKeyword("Len") +
            Each([ Optional(stubKeyword(k)) for k in [ "L", "R", "C" ] ]) -
            Suppress(Literal("/"))
        ).setParseAction(lambda f: IBISNode(f.asDict()))

        paths << (fork_desc | path_pin | path_node | stub_desc)
        super(Path_Description, self).__init__("Path Description", path_pin - paths * (0,), **kwds)

    def fin(self, node):
        super(Path_Description, self).fin(node)
        try:
            node.data[0].pin
        except:
            raise Exception("First item in path is not Pin")

class Board_Description(Section):
    """Section 8."""
    def __init__(self, **kwds):
        kwds["labeled"] = True
        super(Board_Description, self).__init__("Begin Board Description", **kwds)
        self += Keyword("Manufacturer", required=True)
        self += Keyword("Number Of Pins", Integer(check=positive), required=True)
        self += TableSection("Pin List", headers=["signal_name"], required=True)
        self += Path_Description()
        self += DictSection("Reference Designator Map",
            Word(printables)("file_name") -
            restOfLine.setParseAction(lambda f: f[0].strip())("component_name"), asDict=True)
        self += End("End Board Description")


    def fin_path(self, node, name, path):
        for item in path:
            if isinstance(item, dict):
                if "pin" in item:
                    if item.pin.upper() == "NC":
                        item.pin = None
                    if item.pin is not None and item.pin not in node.children.pin_list:
                        raise Exception("Path '{}', pin '{}' not in pin list"
                            .format(name, item.pin))
                elif "node" in item:
                    n, p = item.node.split('.')
                    if n not in node.refmap:
                        raise Exception("Node '{}' of path '{}' references refdes "
                            "not in reference designator map".format(item.node, name))
            elif isinstance(item, list):
                self.fin_path(node, name, item)

    def fin(self, node):
        super(Board_Description, self).fin(node)

        node.refmap = node.children.get("Reference Designator Map", dict())
        for name, path in node.children.path_description.items():
            self.fin_path(node, name, path)
        del node.refmap

class EMI_Component(Section):
    """Section 11."""
    def __init__(self, **kwds):
        super(EMI_Component, self).__init__("Begin EMI Component", **kwds)

        self += Param("Domain", oneof("Digital Analog Digital_Analog"), default="digital")
        self += Param("Cpd", Real(check=positive), delim='=', default=0)
        self += Param("C_Heatsink_gnd", Real(check=positive), delim='=')
        self += Param("C_Heatsink_float", Real(check=positive), delim='=')
        self += Pin_EMI()
        self += Pin_Domain_EMI()
        self += End("End EMI Component")

    def fin(self, node):
        super(EMI_Component, self).fin(node)

        # FIXME: Error should name component.
        if "C_Heatsink_gnd" in node.children and "C_Heatsink_float" in node.children:
            raise Exception("Contains bath 'C_Heatsink_gnd' and 'C_Heatsink_float'")

        domains = set()
        if "Pin EMI" in node.children:
            for pin, item in node.children.pin_emi.items():
                if item.domain_name is not None:
                    domains.add(item.domain_name)

        used = set()
        if "Pin Domain EMI" in node.children:
            for domain, item in node.children.pin_domain_emi.items():
                if domain not in used:
                    raise Exception("Pin Domain EMI domain '{}' not in Pin EMI"
                        .format(domain))
                used.add(domain)

        missing = demains - used
        if len(missing):
            raise Exception("Pin EMI contains '{}' which are not contained in Pin Domain EMI"
                .format(missing))

class Pin_EMI(TableSection):
    def __init__(self, **kwds):
        super(Pin_EMI, self).__init__("Pin EMI", headers=["domain_name", "clock_div"], **kwds)
        self += NAReal(check=positive)("clock_div")

    def fin(self, node):
        super(Pin_EMI, self).fin(node)

        # Fill in defaults.
        for pin, item in node.children.items():
            if item.clock_div is None:
                item.clock_div = 1.0
            if item.domain_name.lower() == "na":
                item.domain_name = None

class Pin_Domain_EMI(TableSection):
    def __init__(self, **kwds):
        super(Pin_Domain_EMI, self).__init__("Pin Domain EMI", headers=["percentage"], **kwds)
        self += Integer(check=in_range(1,100))("percentage")

class EMI_Model(Section):
    def __init__(self, **kwds):
        super(EMI_Model, self).__init__("Begin EMI Model", **kwds)
        self += Param("Model_emi_type", oneof("Ferrite Not_a_ferrite"), default="not_a_ferrite")
        self += Param("Domain", oneof("Analog Digital"))
        self += End("End EMI Model")

class IBISCommon(Parse):
    def __init__(self, comment_holder=[None], **kwds):
        kwds["required"] = True
        super(IBISCommon, self).__init__("body", **kwds)

        self.comment_char = "|"
        self.globals = Parse("globals")
        self.globals += Comment(comment_holder)

        self.header = Header()
        self += self.header
        self += End("End")

class IBS(IBISCommon):
    def __init__(self, **kwds):
        super(IBS, self).__init__(**kwds)
        self += Component()
        self += DictSection("Model Selector", labeled=True)
        self += Model()
        self += Test_Data()
        self += Test_Load()
        self += Submodel()
        self += External_Circuit()
        self += Package_Model()

    def fin(self, node):
        super(IBS, self).fin(node)

        # Make sure all models under a model selector are the same type and exist.
        if "Model Selector" in node.children:
            for name, sel in node.children.model_selector.items():
                if not len(list(sel.keys())):
                    raise Exception("Empty model selector, '{}'".format(name))
                model_type = None
                for model_name in list(sel.keys()):
                    if model_name not in node.children.model:
                        raise Exception("Model '{}' in model selector '{}' does not exist"
                            .format(model_name, name))
                    model = node.children.model[model_name]
                    if model_type:
                        if model_type != model.model_type:
                            raise Exception("Model '{}' in model selector '{}' model_type '{}' does "
                                "not match model type, '{}', of first model, '{}'"
                                .format(model_name, name, model.model_type, model_type, list(sel.keys())[0]))
                
        # Make sure models referenced by the component pin list exist.
        for component_name, component in node.children.component.items():
            for pin_name, pin in component.pin.items():
                model_name = pin.model_name
                if model_name is None or model_name in [ "POWER", "GND", "CIRCUITCALL" ]:
                    continue
                if model_name not in node.children.model:
                    if "Model Selector" in node.children and model_name in node.children.model_selector:
                        model_name = list(node.children.model_selector[model_name].keys())[0]
                    else:
                        raise Exception("Component '{}' lists unknown model '{}' "
                            "for pin '{}'"
                            .format(component_name, model_name, pin_name))
                model_type = node.children.model[model_name].model_type
                if model_type in [ "series", "series_switch" ]:
                    raise Exception("Pin '{}' in component '{}' is "
                        "of wrong model type, '{}'"
                        .format(pin_name, component_name, model_type))


            if "Series Pin Mapping" in component:
                for pin_name, mapping in component.series_pin_mapping.items():
                    model_name = mapping.model_name
                    if model_name not in node.children.model:
                        if model_name in node.children.model_selector:
                            model_name = list(node.children.model_selector[model_name].keys())[0]
                        else:
                            raise Exception("Component '{}' lists unknown model '{}' "
                                "for series pin mapping '{}'"
                                .format(component_name, model_name, pin_name))
                    model_type = node.children.model[model_name].model_type
                    if model_type not in [ "series", "series_switch" ]:
                        raise Exception("Series pin mapping '{}' in component '{}' is "
                            "of wrong model type, '{}'".format(pin_name, component_name, model_type))
                    if model_type == "series" and mapping.function_table_group is not None:
                        raise Exception("Unexpected 'function_table_group' for series "
                            "pin mapping '{}' of component '{}' with model '{}' which "
                            "has model type 'series'"
                            .format(pin_name, component_name, model_name))

        if "Model" in node.children:
            for model_name, model in node.children.model.items():
                model_type = model.model_type
                if "Add Submodel" in model:
                    for submodel_name, mode in model.add_submodel.items():
                        if submodel_name not in node.children.submodel:
                            raise Exception("Submodel '{}' listed under model '{}' not available"
                                .format(submodel_name, model_name))
                if "Driver Schedule" in model:
                    for driver, data in model.driver_schedule.items():
                        if driver not in node.children.model:
                            raise Exception("Listed driver schedule model, '{}', of '{}' does not exist"
                                .format(driver, model_name))
                        sched_model = node.children.model[driver]
                        if "Driver Schedule" is sched_model:
                            raise Exception("Listed driver schedule model, '{}' of '{}' contains "
                                "driver schedule section"
                                .format(driver, model_name))

        if "Test Data" in node.children:
            for data_name, data in node.children.test_data.items():
                model_name = data.driver_model
                if model_name not in node.children.model:
                    raise Exception("Test data '{}' references non-existent model '{}'"
                        .format(data_name, model_name))
                if "Driver_model_inv" in data:
                    inv_model_name = data.driver_model_inv
                    if inv_model_name not in node.children.model:
                        raise Exception("Test data '{}' references non-existent model '{}'"
                            .format(data_name, inv_model_name))
                load_name = data.test_load
                if load_name not in node.children.test_load:
                    raise Exception("Test data '{}' references non-existent load '{}'"
                        .format(data_name, load_name))
                load = node.children.test_load[load_name]
                if data.test_data_type != load.test_load_type:
                    raise Exception("Test data '{}' type does not match load ('{}') type"
                        .format(data_name, load_name))

        if "Test Load" in node.children:
            for load_name, load in node.children.test_load.items():
                model_name = data.receiver_model
                if model_name not in node.children.model:
                    raise Exception("Test load '{}' references non-existent model '{}'"
                        .format(load_name, model_name))
                if "Receiver_model_inv" in data:
                    inv_model_name = data.receiver_model_inv
                    if inv_model_name not in node.children.model:
                        raise Exception("Test load '{}' references non-existent model '{}'"
                            .format(load_name, inv_model_name))


class PKG(IBISCommon):
    def __init__(self, **kwds):
        super(PKG, self).__init__(**kwds)
        self += Package_Model()

class EBD(IBISCommon):
    def __init__(self, **kwds):
        super(EBD, self).__init__(**kwds)
        self += Board_Description()

class Parser(object):
    def __init__(self, root):
        self.root = root

    def reset(self):
        self.comment_holder[0] = "|"

    def initial(self):
        # Start on fake "header" object, escape to body object.
        self.root_node = self.root.initial(None, None)
        header_node = self.root.header.initial(None, None)
        header_node.parent = self.root_node
        return header_node

    def parse(self, input_file):
        self.reset()
        self.current = self.initial()

        if isinstance(input_file, str):
            input_file = open(input_file, 'rb')
        num = 1
        while True:
            line = input_file.readline()
            if line == "": # EOF
                if self.current:
                    raise Exception("No [End] keyword")
                return self.root_node.children
            try:
                self.parseLine(line)
            except:
                # FIXME: Maybe add outside of scope message for otherwise valid stuff.
                print("Parsing failed on line {}: '{}'".format(num, line.rstrip()))
                raise
            num += 1

    def backtrace(self):
        objs = []
        curr = self.current
        while curr is not None:
            objs.insert(0, curr)
            curr = curr.parent
        first = True
        for obj in objs:
            if first:
                print("In", end=' ')
                first = False
            else:
                print(",", end=' ')
            print("'{}".format(obj.parser.key), end=' ')
            if hasattr(obj, 'key') and obj.key is not None:
                print("{}'".format(obj.key), end=' ')
            else:
                print("'", end=' ')
        print(":")

    def parseLine(self, line):
        text, _, comment = line.partition(self.comment_holder[0])
        text = text.rstrip()
        comment = comment.strip()

        # We are past '[End]', only allow empty lines.
        if not self.current:
            if len(text):
                raise Exception("Error: Garbage past end of file")
            else:
                return

        last = self.current.parser
        while True:
            child = self.current.parser.find_parser(text)
            if child is not None:
                parsed = True
                node = child.initial(text, comment)
                if node is not None:
                    node.parent = self.current
                    self.current = node
                else:
                    # None child means go up (end marker).
                    try:
                        self.current.parser.fin(self.current)
                    except:
                        self.backtrace()
                        raise
                    self.current = self.current.parent
                break

            # Treat lines starting with '[' as "special".
            elif text[:1] != '[':
                try:
                    if self.current.parser.parse(self.current, text.lstrip(), comment):
                        break
                except:
                    print('Parse Error', end=' ')
                    self.backtrace()
                    raise

            try:
                self.current.parser.fin(self.current)
            except:
                self.backtrace()
                raise

            self.current = self.current.parent
            if not isinstance(last, Section) and self.current:
                last = self.current.parser

            if not self.current:
                raise Exception("Error: Unexpected text in section [{}]".format(last.key))

class IBSParser(Parser):
    def __init__(self):
        self.comment_holder = [ "|" ]
        super(IBSParser, self).__init__(IBS(comment_holder=self.comment_holder))

class PKGParser(Parser):
    def __init__(self):
        self.comment_holder = [ "|" ]
        super(PKGParser, self).__init__(PKG(comment_holder=self.comment_holder))

class EBDParser(Parser):
    def __init__(self):
        self.comment_holder = [ "|" ]
        super(EBDParser, self).__init__(EBD(comment_holder=self.comment_holder))

def dump(node, depth=0):
    if isinstance(node, dict):
        print("{")
        for name, child in node.items():
            print("{}{}:".format("  " * depth, name), end=' ')
            dump(child, depth + 1)
        print("{}}},".format("  " * (depth - 1)))
    elif isinstance(node, list):
        print("[")
        for child in node:
            print("{} ".format("  " * depth), end=' ')
            dump(child, depth + 1)
        print("{}]".format("  " * depth))
    else:
        print("'{}'".format(node))

if __name__ == "__main__":
    f = sys.argv[1]
    n, p, ext = f.rpartition(".")

    ext = ext.lower()
    if ext == "ibs":
        parser = IBSParser()
    elif ext == "pkg":
        parser = PKGParser()
    elif ext == "ebd":
        parser = EBDParser()
    else:
        print("Unknown extensions, '{}', assuming ibs".format(ext))
        parser = IBSParser()

    root = parser.parse(open(sys.argv[1], 'r'))
    dump(root)
