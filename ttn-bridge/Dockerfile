FROM node:10

RUN mkdir -p /opt/node-app && \
    chown node:node /opt/node-app

WORKDIR /opt/node-app

USER node

COPY package*.json ./

RUN npm install --no-optional && \
    npm cache clean --force

COPY start.js ./
COPY app.js ./

CMD [ "node", "start.js" ]