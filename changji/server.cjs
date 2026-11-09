const http = require('node:http');
const fs = require('node:fs/promises');
const path = require('node:path');

const root = path.join(__dirname, 'dist');
const localCourses = new Set(['/words.js', '/words4.js', '/words-basic.js']);
const types = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.ttf': 'font/ttf',
  '.txt': 'text/plain; charset=utf-8'
};

http.createServer(async (req, res) => {
  let pathname;
  try {
    pathname = decodeURIComponent(new URL(req.url, 'http://localhost').pathname);
  } catch {
    res.writeHead(400).end('Bad request');
    return;
  }
  const file = path.resolve(root, '.' + (pathname === '/' ? '/index.html' : pathname));
  if (!file.startsWith(root + path.sep)) {
    res.writeHead(403).end();
    return;
  }
  try {
    let data;
    if (localCourses.has(pathname)) {
      const local = path.join(__dirname, '.local-vocabulary', path.basename(pathname));
      try {
        data = await fs.readFile(local);
      } catch (error) {
        if (error.code !== 'ENOENT') throw error;
      }
      res.setHeader('Cache-Control', 'no-store');
    }
    data ??= await fs.readFile(file);
    res.setHeader('Content-Type', types[path.extname(file)] || 'application/octet-stream');
    res.end(data);
  } catch {
    res.writeHead(404).end('Not found');
  }
}).listen(4176, '127.0.0.1', () => console.log('Local: http://127.0.0.1:4176'));
