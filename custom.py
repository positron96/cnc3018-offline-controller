import sys

Import("env")

board_config = env.BoardConfig()


path='debug.tools.stlink.server.arguments'
t = board_config.get(path)
board_config.update(path, ['-c', 'set FLASH_SIZE 0x20000', 'preved'] + t)
board_config.update('debug.openocd_extra_args', ['prevedmedved'])


with open('ttt.txt', 'a') as f:
    #f.write( str(board_config.__dict__) )
    import json, sys
    json.dump(sys.argv, f, indent=4)
    json.dump(board_config.get(path), f, indent=4)

print( env.GetBuildType() )


