import textwrap
import os


def error(msg):
    print msg
    exit(1)


entry_stack = []


class Entry(object):

    def __init__(self):
        self.brief_ = ''
        self.body_ = []
        self.params_ = []
        self.return_ = ''
        self.code_ = []
        self.is_class_ = False

    def on_brief(self, line):
        line = line.replace('/// @brief', '')
        line = line.strip(' \t\r\n')
        self.brief_ = line

    def on_param(self, line):
        line = line.replace('/// @param', '')
        line = line.strip(' \t\r\n')
        self.params_.append(line)

    def on_return(self, line):
        line = line.replace('/// @return', '')
        line = line.strip(' \t\r\n')
        self.return_ = line

    def on_body(self, line):
        line = line.replace('///', '')
        line = line.strip(' \t\r\n')
        self.body_.append(line)

    def on_code(self, line):
        line = line.strip(' \t\r\n')
        if line.startswith('class') or line.startswith('struct'):
            if ';' not in line:
                self.is_class_ = True
                entry_stack.append(self)
        items = line.split(';{)')
        if len(items) > 1:
            line = items[0]
        line = line.strip(';{ \t\r\n')
        self.code_.append(line)

    def write(self, fd, line):
        fd.write('{0}{1}'.format(line, '\n'))

    def eol(self, fd):
        fd.write('\n')

    def emit(self, fd):

        # TITLE
        if self.is_class_:
            self.write(fd, '----')
            self.write(fd, '## {0}'.format(self.code_[0]))
            self.eol(fd)
        elif self.code_:
            self.write(fd, '****')
            self.write(fd, '```c')
            for c in self.code_:
                self.write(fd, c)
            self.write(fd, '```')
            self.eol(fd)
        else:
            self.write(fd, '----')
            # prep brief to become a main title
            fd.write('# ')

        # BRIEF
        if self.brief_:
            self.write(fd, '{0}'.format(self.brief_))
            self.eol(fd)

        # PARAMS
        if self.params_:
            self.write(fd, 'Params:')
            for p in self.params_:
                self.write(fd, '- {0}'.format(p))
            self.eol(fd)

        # RETURN
        if self.return_:
            self.write(fd, 'Return:')
            self.write(fd, '- {0}'.format(self.return_))
            self.eol(fd)

        # BODY
        if self.body_:
            body = ' '.join(self.body_).strip(' \t\r\n')
            body = textwrap.wrap(body, 80)
            for b in body:
                self.write(fd, '{0}'.format(b))
            self.eol(fd)

        # LINE BREAK
        self.eol(fd)


def parse(fd_in, fd_out):
    entry = None
    for line in fd_in.readlines():
        line = line.strip(' \r\n\t')

        if line.startswith('/// @brief'):
            assert not entry
            entry = Entry()
            entry.on_brief(line)
            continue

        if line.startswith('/// @param'):
            assert entry
            entry.on_param(line)
            continue

        if line.startswith('/// @return'):
            assert entry
            entry.on_return(line)
            continue

        if entry and line.startswith('/// @end'):
            entry.emit(fd_out)
            entry = None
            continue

        if entry and line.startswith('///'):
            assert entry
            entry.on_body(line)
            continue

        if line.startswith('};'):
            assert (len(entry_stack) > 0)
            entry_stack.pop()
            continue

        if entry is not None:
            entry.on_code(line)
            if ';' in line or '{' in line or ')' in line:
                entry.emit(fd_out)
                entry = None


def main(path_in, path_out):
    fd_in = open(path_in, "r")
    if not fd_in:
        error('unable to open \'{0}\' for reading'.format(path_in))
    fd_out = open(path_out, "w")
    if not fd_out:
        error('unable to open \'{0}\' for writing'.format(path_out))
    parse(fd_in, fd_out)


if __name__ == '__main__':
    main('cmd.h', '../README.md')
