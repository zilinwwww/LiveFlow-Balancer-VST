import { getUserFromRequest, json, type Env } from '../_utils';

export const onRequestGet: PagesFunction<Env> = async (context) => {
  const user = await getUserFromRequest(context.request, context.env);
  if (!user) return json({ status: 'error', code: 'UNAUTHORIZED' }, 401);
  return json({ status: 'ok', user });
};
