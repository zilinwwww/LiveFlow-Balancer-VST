import { getUserFromRequest, generateLicenseKey, json, type Env } from '../_utils';

export const onRequestPost: PagesFunction<Env> = async (context) => {
  try {
    const user = await getUserFromRequest(context.request, context.env);
    if (!user) return json({ status: 'error', code: 'UNAUTHORIZED' }, 401);

    const { product } = await context.request.json<{ product: string }>();
    if (!product) return json({ status: 'error', code: 'MISSING_PRODUCT' }, 400);

    // Check if user already has a license for this product
    const existing = await context.env.DB.prepare(
      'SELECT id FROM licenses WHERE user_id = ? AND product = ?'
    ).bind(user.id, product).first();

    if (existing) return json({ status: 'error', code: 'ALREADY_CLAIMED' }, 409);

    const id = crypto.randomUUID();
    const key = generateLicenseKey();

    await context.env.DB.prepare(
      'INSERT INTO licenses (id, user_id, product, key, status) VALUES (?, ?, ?, ?, ?)'
    ).bind(id, user.id, product, key, 'available').run();

    return json({ status: 'ok', license: { id, product, key, status: 'available' } }, 201);
  } catch (e: any) {
    return json({ status: 'error', message: e.message }, 500);
  }
};
