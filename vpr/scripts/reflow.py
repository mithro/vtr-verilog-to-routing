#!/usr/bin/env python3

import sys, os
import argparse

def reflow(f, o):
    space_string = ''

    for line in f:
        # Skip lines with // comments
        if space_string == '' and line.find('//') != -1:
            o.write(line)
            continue

        sc = line.find("/*")
        ec = line.find("*/")

        # Skip /* foo bar */ comments
        if sc != -1 and ec != -1:
            o.write(line)
        else:
            # Start of /* comment */
            if sc != -1:
                space_string = (''.join(' ' for i in range(sc+1)))
                o.write(line)
            elif space_string != '':
                # Already in /* comment */
                ls = line.lstrip()

                # Empty line, write aligned comment prefix
                if len(ls) == 0:
                    o.write(space_string + "*" + "\n")
                elif ls[0] != '*':
                    #
                    # Fix comments like '********* sth **********'
                    # to look like ' * ********** sth **********'
                    #
                    o.write(space_string + "* " + ls)
                else:
                    # Rest of the /* comment */
                    o.write(space_string + ls)
            else:
                # Outside /* comment */
                o.write(line)

        # Moving outside /* comment */
        if ec != -1:
            space_string = ''

def main(argv):
    parser = argparse.ArgumentParser(description='Reflow C like box comments')
    parser.add_argument('--inplace', dest='inplace', action='store_true',
        help='Inplace edit specified file')
    parser.add_argument('--input', dest='inputfile', action='store',
        help='Input file')
    parser.add_argument('--output', dest='outputfile', action='store',
        help='Output file')
    args = parser.parse_args(argv[1:]);

    fin = open(args.inputfile, 'r')

    if args.inplace:
        fout = open(args.inputfile + '.bak', 'w')
    else:
        fout = open(args.outputfile, 'w');

    reflow(fin, fout)
    fin.close();
    fout.close();

    if args.inplace:
        os.rename(args.inputfile + '.bak', args.inputfile)

if __name__ == "__main__":
    main(sys.argv)
