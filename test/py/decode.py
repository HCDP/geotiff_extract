import struct

def lzw_decode(encoded, buffersize=0, out=None):
    r"""Decompress LZW (Lempel-Ziv-Welch) encoded TIFF strip (byte string).
    The strip must begin with a CLEAR code and end with an EOI code.
    This implementation of the LZW decoding algorithm is described in TIFF v6
    and is not compatible with old style LZW compressed files like
    quad-lzw.tif.
    >>> lzw_decode(b'\x80\x1c\xcc\'\x91\x01\xa0\xc2m6\x99NB\x03\xc9\xbe\x0b'
    ...            b'\x07\x84\xc2\xcd\xa68|"\x14 3\xc3\xa0\xd1c\x94\x02\x02')
    b'say hammer yo hammer mc hammer go hammer'
    """
    codes = None
    with open("test/codes.txt") as f:
        codes = [line.strip() for line in f]

    print(codes)


    len_encoded = len(encoded)
    bitcount_max = len_encoded * 8
    unpack = struct.unpack
    table = [bytes([i]) for i in range(256)]
    table.extend((b'\x00', b'\x00'))


    code_i = 0
    def next_code():
        nonlocal code_i
        #print("next code")
        # return integer of 'bitw' bits at 'bitcount' position in encoded
        start = bitcount // 8
        s = encoded[start : start + 4]
        # s = s[::-1]
        # print(s, hex(s[0]), hex(s[1]), hex(s[2]), hex(s[3]))
        try:
            code = unpack('>I', s)[0]
            #print(unpack('>I', s))
            #print("no exception")
        except Exception as e:
            print(e)
            code = unpack('>I', s + b'\x00' * (4 - len(s)))[0]
        code <<= bitcount % 8
        code &= mask
        code >>= shr
        if str(code) != codes[code_i]:
            print(code, codes[code_i])
        code_i += 1
        return code

    switchbits = {  # code: bit-width, shr-bits, bit-mask
        255: (9, 23, int(9 * '1' + '0' * 23, 2)),
        511: (10, 22, int(10 * '1' + '0' * 22, 2)),
        1023: (11, 21, int(11 * '1' + '0' * 21, 2)),
        2047: (12, 20, int(12 * '1' + '0' * 20, 2)),
    }
    #print(switchbits)
    bitw, shr, mask = switchbits[255]
    bitcount = 0

    if len_encoded < 4:
        raise ValueError('strip must be at least 4 characters long')

    # if next_code() != 256:
    #     return b'0000'
    #     raise ValueError('strip must begin with CLEAR code')

    code = 0
    oldcode = 0
    result = []
    result_append = result.append
    while True:
        code = next_code()  # ~5% faster when inlining this function
        bitcount += bitw
        if code == 257 or bitcount >= bitcount_max:  # EOI
            print(bitcount, bitcount_max)
            break
        if code == 256:  # CLEAR

            table = table[:258]
            table_append = table.append
            bitw, shr, mask = switchbits[255]
            code = next_code()
            bitcount += bitw
            if code == 257:  # EOI
                break
            result_append(table[code])
        else:
            if code < len(table):
                decoded = table[code]
                #use byte vectors for table contents for easy appending
                newcode = table[oldcode] + decoded[:1]
                #gets first byte
                #print(decoded[:1])
            else:
                newcode = table[oldcode]
                newcode += newcode[:1]
                decoded = newcode
            result_append(decoded)
            table_append(newcode)
        oldcode = code
        if len(table) in switchbits:
            bitw, shr, mask = switchbits[len(table)]

    if code != 257:
        # logging.warning(f'unexpected end of LZW stream (code {code!r})')
        pass

    return b''.join(result)