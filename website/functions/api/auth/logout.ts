import { json } from '../_utils';

export const onRequestPost: PagesFunction = async () => {
  return json({ status: 'ok' }, 200, {
    'Set-Cookie': 'token=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0',
  });
};
