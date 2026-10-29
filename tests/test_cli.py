import os
import socket
import subprocess
import tempfile
import threading
import time
import unittest
from pathlib import Path


class CLI受け入れテスト(unittest.TestCase):
    def test_存在しないマップはエラーで終了する(self):
        result = subprocess.run([os.environ['CHASER_SERVER_BINARY'], '/not-found.map', '--headless'], capture_output=True, timeout=5)
        self.assertEqual(result.returncode, 2)

    def test_ポート使用中はエラーで終了する(self):
        with tempfile.TemporaryDirectory() as directory, socket.socket() as listener:
            listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            listener.bind(('0.0.0.0', 2009)); listener.listen()
            path = Path(directory) / 'test.map'
            path.write_text('N:test\nT:2\nS:15,17\n' + ('D:' + ','.join(['0'] * 15) + '\n') * 17 + 'C:1,1\nH:13,15\n')
            result = subprocess.run([os.environ['CHASER_SERVER_BINARY'], str(path), '--headless'], cwd=directory, capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 2)

    def test_固定マップで対戦し勝敗を出力する(self):
        binary = os.environ['CHASER_SERVER_BINARY']
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'test.map'
            path.write_text('N:test\nT:2\nS:15,17\n' + ('D:' + ','.join(['0'] * 15) + '\n') * 17 + 'C:1,1\nH:13,15\n')
            env = dict(os.environ, QT_QPA_PLATFORM='offscreen')
            server = subprocess.Popen([binary, str(path), '--headless'], cwd=directory, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            errors = []
            def client(port):
                try:
                    for _ in range(100):
                        sock = socket.socket(); sock.settimeout(15)
                        try: sock.connect(('127.0.0.1', port)); break
                        except OSError: sock.close(); time.sleep(.1)
                    else: raise RuntimeError('接続できません')
                    with sock:
                        sock.sendall(b'test\r\n')
                        stream = sock.makefile('rb')
                        while True:
                            line = stream.readline()
                            if not line: break
                            if line != b'@\r\n': raise RuntimeError(repr(line))
                            sock.sendall(b'gr\r\n')
                            ready = stream.readline()
                            if not ready: break
                            sock.sendall(b'lu\r\n')
                            if ready.startswith(b'0'): break
                            action = stream.readline()
                            if not action: break
                            sock.sendall(b'#\r\n')
                            if action.startswith(b'0'): break
                except Exception as exc: errors.append(exc)
            threads = [threading.Thread(target=client, args=(port,)) for port in (2009, 2010)]
            try:
                for thread in threads: thread.start()
                stdout, stderr = server.communicate(timeout=35)
                for thread in threads: thread.join(timeout=2)
                self.assertFalse(errors, errors)
                self.assertEqual(server.returncode, 0, stderr)
                self.assertIn('RESULT DRAW 0 0', stdout)
            finally:
                if server.poll() is None: server.kill(); server.communicate()

if __name__ == '__main__': unittest.main()
