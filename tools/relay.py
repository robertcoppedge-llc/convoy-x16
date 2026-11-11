"""Pair two Convoy machines on the LAN.

Run this on one PC, then point both games at that PC's address
(title screen, 3, type the address, Enter). Port 6416.

    python tools/relay.py
"""
import socket
import threading

PORT = 6416


def pipe(src, dst):
    try:
        while True:
            data = src.recv(512)
            if not data:
                break
            dst.sendall(data)
    except OSError:
        pass
    finally:
        try:
            dst.shutdown(socket.SHUT_WR)
        except OSError:
            pass


def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("0.0.0.0", PORT))
    server.listen(2)
    print("convoy relay listening on 0.0.0.0:%d" % PORT)
    a, aa = server.accept()
    print("first side", aa)
    b, bb = server.accept()
    print("second side", bb)
    threading.Thread(target=pipe, args=(a, b), daemon=True).start()
    threading.Thread(target=pipe, args=(b, a), daemon=True).start()
    try:
        threading.Event().wait()
    except KeyboardInterrupt:
        pass
    a.close()
    b.close()
    server.close()


if __name__ == "__main__":
    main()
