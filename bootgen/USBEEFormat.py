import codecs

class USBEEFormat(object):
    def to_dict(self):
        return {}
    
    @staticmethod
    def string_to_BOM_string(string_to_encode):
        try:
            encoded_description = string_to_encode.encode('ascii')
        except:
            try:
                encoded_description = codecs.BOM_UTF8 + string_to_encode.encode('utf-8')
            except:
                try:
                    encoded_description = codecs.BOM_UTF16_BE + string_to_encode.encode('utf-16-be')
                except:
                    try:
                        encoded_description = codecs.BOM_UTF32_BE + string_to_encode.encode('utf-32-be')
                    except:
                        raise UnicodeEncodeError('Cannot encode text in ASCII or UTF-8 or UTF-16 or UTF-32')
        return encoded_description
